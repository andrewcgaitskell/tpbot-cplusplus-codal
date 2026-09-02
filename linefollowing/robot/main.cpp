#include "MicroBit.h"
#include "tpbot.h"
#include "tpbotprotocol.h"

MicroBit uBit;
TPBot robot(uBit);

// This robot's ID. Change per-robot if you run more than one on the
// same radio group. RobotID 0 in an incoming packet is treated as
// "broadcast to all robots" and is always acted on.
static const uint8_t THIS_ROBOT_ID = 1;

// --- Bang-bang line following, no PID ---
//
// Two sensors, each binary (on-line / off-line). No proportional term:
// every correction is a fixed, maximal turn. The two problems this
// version of bang-bang normally has are (a) violent zigzag, and
// (b) guessing wrong when both sensors lose the line simultaneously
// (e.g. at a gap or sharp corner). This refactor addresses (b) by
// tracking the *last known error direction* and continuing to search
// that way rather than stopping blind, and softens (a) slightly by
// giving "straight" its own state instead of re-deciding every loop.
//
// Concurrency: line following and the radio diagnostic link run on
// separate CODAL fibers. Previously both lived in one loop and the
// comment on the old version noted this was "to answer polls without
// blocking line following" - splitting them onto fibers makes that
// guarantee structural rather than incidental, and means motor timing
// doesn't jitter if a radio packet arrives mid-turn.

enum class FollowState
{
    Straight, // both sensors on line - drive ahead
    CorrectingLeft, // left sensor only - pivot left
    CorrectingRight, // right sensor only - pivot right
    SearchingLeft, // line lost, last known error was left - keep pivoting left
    SearchingRight, // line lost, last known error was right - keep pivoting right
    Stopped // line lost with no direction history, or lost too long
};

enum class Direction : int8_t
{
    Unknown = 0,
    Left = 1,
    Right = -1
};

// --- Tuning ---
static const int BASE_SPEED = 15;
static const int CORRECT_OUTER = 22; // outer wheel speed during a live correction - a nudge, not a spin
static const int CORRECT_INNER = 6;  // inner wheel speed during a live correction (still forward, just slower)
static const int SEARCH_OUTER = 40; // once the line is fully lost, turn harder to relocate it
static const int SEARCH_INNER = -10; // small reverse only once we're actively hunting, not on every correction
static const int TRIM = 2; // constant bias correcting mechanical left-drift, applied only when going straight
static const int MIN_SPEED = -25;
static const int MAX_SPEED = 100;
static const int LOST_LINE_TIMEOUT_MS = 600; // give up searching and stop after this long with no line
static const int LOOP_DELAY_MS = 20;
static const int DEBOUNCE_READS = 2; // consecutive matching readings required before a state change is accepted

// --- Shared state between fibers ---
// Single-word volatiles are effectively atomic on the microbit's Cortex-M0,
// so no explicit lock is used here. If more fields are added, or any of
// them stop being single-word, protect updates with a critical section
// (e.g. target_disable_irq()/target_enable_irq() around the read-modify-write).
static volatile FollowState g_state = FollowState::Stopped;
static volatile Direction g_lastKnownDirection = Direction::Unknown;
static volatile bool g_lineEverSeen = false;

// Loop timing telemetry, read by the radio fiber and reported to base on
// request. Two values are kept: the most recent loop time, and the worst
// (max) seen since boot - averages hide exactly the spikes you're trying
// to find, so the max is usually the more useful number.
static volatile uint16_t g_lastLoopMs = 0;
static volatile uint16_t g_maxLoopMs = 0;

static int clampSpeed(int v)
{
    if (v < MIN_SPEED) return MIN_SPEED;
    if (v > MAX_SPEED) return MAX_SPEED;
    return v;
}

// Pure state-transition function: sensor readings + history in, new state out.
// Kept separate from motor output so the logic can be reasoned about (and
// unit-tested off-device) without touching hardware.
//
// Debounced: a raw reading has to repeat DEBOUNCE_READS times in a row
// before it's accepted as a real state change. Without this, ordinary
// sensor flicker at the line edge on a straight run was enough to flip
// direction every loop, firing full-force corrections back and forth -
// that's the "violent veering", not a genuine drift needing correction.
static FollowState rawStateFromSensors(bool leftBlack, bool rightBlack, int msSinceLineSeen)
{
    if (leftBlack && rightBlack)
    {
        g_lastKnownDirection = Direction::Unknown; // centred - no error to remember
        return FollowState::Straight;
    }

    if (leftBlack && !rightBlack)
    {
        g_lastKnownDirection = Direction::Left;
        return FollowState::CorrectingLeft;
    }

    if (!leftBlack && rightBlack)
    {
        g_lastKnownDirection = Direction::Right;
        return FollowState::CorrectingRight;
    }

    // Neither sensor sees the line - fall back on history.
    if (msSinceLineSeen > LOST_LINE_TIMEOUT_MS)
        return FollowState::Stopped;

    switch (g_lastKnownDirection)
    {
        case Direction::Left:  return FollowState::SearchingLeft;
        case Direction::Right: return FollowState::SearchingRight;
        default:               return FollowState::Stopped; // never had a fix - nothing to search toward
    }
}

static FollowState nextState(bool leftBlack, bool rightBlack, int msSinceLineSeen)
{
    static FollowState pendingState = FollowState::Stopped;
    static FollowState acceptedState = FollowState::Stopped;
    static int matchCount = 0;

    FollowState raw = rawStateFromSensors(leftBlack, rightBlack, msSinceLineSeen);

    // Searching/Stopped are already a response to sustained absence of the
    // line (msSinceLineSeen), not a single noisy read - apply them
    // immediately rather than waiting for them to debounce too.
    if (raw == FollowState::SearchingLeft || raw == FollowState::SearchingRight || raw == FollowState::Stopped)
    {
        acceptedState = raw;
        pendingState = raw;
        matchCount = 0;
        return acceptedState;
    }

    if (raw == pendingState)
    {
        matchCount++;
    }
    else
    {
        pendingState = raw;
        matchCount = 1;
    }

    if (matchCount >= DEBOUNCE_READS)
        acceptedState = pendingState;

    return acceptedState;
}

// Turn convention matches the earlier working bang-bang version:
// error toward the left sensor -> pivot left, and vice versa.
//
// Live corrections (CorrectingLeft/Right) are now a nudge - the inner
// wheel keeps moving forward, just slower - rather than a full reversal.
// The harder, reversing turn is reserved for Searching states, where the
// line is genuinely gone and a tighter turn to relocate it is worth the
// extra aggressiveness.
static void driveForState(FollowState state)
{
    switch (state)
    {
        case FollowState::Straight:
            robot.setWheels(clampSpeed(BASE_SPEED + TRIM), clampSpeed(BASE_SPEED - TRIM));
            break;

        case FollowState::CorrectingLeft:
            robot.setWheels(clampSpeed(CORRECT_INNER), clampSpeed(CORRECT_OUTER));
            break;

        case FollowState::CorrectingRight:
            robot.setWheels(clampSpeed(CORRECT_OUTER), clampSpeed(CORRECT_INNER));
            break;

        case FollowState::SearchingLeft:
            robot.setWheels(clampSpeed(SEARCH_INNER), clampSpeed(SEARCH_OUTER));
            break;

        case FollowState::SearchingRight:
            robot.setWheels(clampSpeed(SEARCH_OUTER), clampSpeed(SEARCH_INNER));
            break;

        case FollowState::Stopped:
        default:
            robot.stopCar();
            break;
    }
}

// --- Fiber 1: line following ---
// Samples sensors, updates the state machine, drives the motors. Runs
// continuously at LOOP_DELAY_MS regardless of what the radio fiber is doing.
static void lineFollowFiber()
{
    int msSinceLineSeen = 0;
    unsigned long lastTick = uBit.systemTime();

    while (1)
    {
        unsigned long now = uBit.systemTime();
        uint16_t elapsed = (uint16_t)(now - lastTick); // actual time since the previous iteration started
        lastTick = now;
        g_lastLoopMs = elapsed;
        if (elapsed > g_maxLoopMs)
            g_maxLoopMs = elapsed;

        bool leftBlack = robot.trackSide(LineSide::Left, LineState::Black);
        bool rightBlack = robot.trackSide(LineSide::Right, LineState::Black);

        if (leftBlack || rightBlack)
        {
            msSinceLineSeen = 0;
            g_lineEverSeen = true;
        }
        else
        {
            msSinceLineSeen += LOOP_DELAY_MS;
        }

        g_state = nextState(leftBlack, rightBlack, msSinceLineSeen);
        driveForState(g_state);

        uBit.sleep(LOOP_DELAY_MS);
    }
}

// --- Fiber 2: radio / diagnostics ---
// Answers diagnostic polls from the base station independently of the
// line-following cadence above - a slow or busy radio call here no longer
// has any chance of delaying a motor correction.
static void radioFiber()
{
    while (1)
    {
        uint8_t rx[32];
        int n = uBit.radio.datagram.recv(rx, 31);

        if (n > 0)
        {
            RobotCommand cmd;
            if (unpackRobotCommand(rx, n, cmd) &&
                (cmd.robotId == THIS_ROBOT_ID || cmd.robotId == 0))
            {
                if (cmd.command == CMD_POLL_SENSORS)
                {
                    bool leftBlack = robot.trackSide(LineSide::Left, LineState::Black);
                    bool rightBlack = robot.trackSide(LineSide::Right, LineState::Black);

                    uint8_t tx[ROBOT_COMMAND_SIZE];
                    int len = packRobotCommand(tx, THIS_ROBOT_ID, CMD_SENSOR_REPORT,
                                               leftBlack ? 1 : 0, rightBlack ? 1 : 0);
                    uBit.radio.datagram.send(tx, len);
                }
                else if (cmd.command == CMD_POLL_LOOP_TIME)
                {
                    uint8_t tx[ROBOT_COMMAND_SIZE];
                    int len = packRobotCommand(tx, THIS_ROBOT_ID, CMD_LOOP_TIME_REPORT,
                                               (int16_t)g_lastLoopMs, (int16_t)g_maxLoopMs);
                    uBit.radio.datagram.send(tx, len);
                }
            }
        }

        uBit.sleep(10);
    }
}

int main()
{
    uBit.init();
    uBit.radio.enable();
    uBit.radio.setGroup(1);

    create_fiber(lineFollowFiber);
    create_fiber(radioFiber);

    // Main fiber has nothing left to do; the two worker fibers above run
    // the robot. Sleeping here (rather than returning) keeps it parked
    // in the scheduler.
    while (1)
        uBit.sleep(1000);
}
