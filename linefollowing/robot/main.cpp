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
static const int TURN_SPEED = 60; // outer wheel speed during a hard turn
static const int PIVOT_SPEED = -25; // inner wheel speed during a hard turn (negative = reverse, tighter pivot)
static const int SEARCH_SPEED = 40; // slightly gentler than a live correction, since we're guessing
static const int TRIM = 2; // constant bias correcting mechanical left-drift, applied only when going straight
static const int MIN_SPEED = -25;
static const int MAX_SPEED = 100;
static const int LOST_LINE_TIMEOUT_MS = 600; // give up searching and stop after this long with no line
static const int LOOP_DELAY_MS = 20;

// --- Shared state between fibers ---
// Single-word volatiles are effectively atomic on the microbit's Cortex-M0,
// so no explicit lock is used here. If more fields are added, or any of
// them stop being single-word, protect updates with a critical section
// (e.g. target_disable_irq()/target_enable_irq() around the read-modify-write).
static volatile FollowState g_state = FollowState::Stopped;
static volatile Direction g_lastKnownDirection = Direction::Unknown;
static volatile bool g_lineEverSeen = false;

static int clampSpeed(int v)
{
    if (v < MIN_SPEED) return MIN_SPEED;
    if (v > MAX_SPEED) return MAX_SPEED;
    return v;
}

// Pure state-transition function: sensor readings + history in, new state out.
// Kept separate from motor output so the logic can be reasoned about (and
// unit-tested off-device) without touching hardware.
static FollowState nextState(bool leftBlack, bool rightBlack, int msSinceLineSeen)
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

// Turn convention matches the earlier working bang-bang version:
// error toward the left sensor -> pivot left, and vice versa.
static void driveForState(FollowState state)
{
    switch (state)
    {
        case FollowState::Straight:
            robot.setWheels(clampSpeed(BASE_SPEED + TRIM), clampSpeed(BASE_SPEED - TRIM));
            break;

        case FollowState::CorrectingLeft:
            robot.setWheels(clampSpeed(PIVOT_SPEED), clampSpeed(TURN_SPEED));
            break;

        case FollowState::CorrectingRight:
            robot.setWheels(clampSpeed(TURN_SPEED), clampSpeed(PIVOT_SPEED));
            break;

        case FollowState::SearchingLeft:
            robot.setWheels(clampSpeed(-SEARCH_SPEED / 2), clampSpeed(SEARCH_SPEED));
            break;

        case FollowState::SearchingRight:
            robot.setWheels(clampSpeed(SEARCH_SPEED), clampSpeed(-SEARCH_SPEED / 2));
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

    while (1)
    {
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
                (cmd.robotId == THIS_ROBOT_ID || cmd.robotId == 0) &&
                cmd.command == CMD_POLL_SENSORS)
            {
                bool leftBlack = robot.trackSide(LineSide::Left, LineState::Black);
                bool rightBlack = robot.trackSide(LineSide::Right, LineState::Black);

                uint8_t tx[ROBOT_COMMAND_SIZE];
                int len = packRobotCommand(tx, THIS_ROBOT_ID, CMD_SENSOR_REPORT,
                                           leftBlack ? 1 : 0, rightBlack ? 1 : 0);
                uBit.radio.datagram.send(tx, len);
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
