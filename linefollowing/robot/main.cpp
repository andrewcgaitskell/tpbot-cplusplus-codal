#include "MicroBit.h"
#include "tpbot.h"
#include "tpbotprotocol.h"

MicroBit uBit;
TPBot robot(uBit);

// This robot's ID. Change per-robot if you run more than one on the
// same radio group. RobotID 0 in an incoming packet is treated as
// "broadcast to all robots" and is always acted on.

static const uint8_t THIS_ROBOT_ID = 1;

// --- PID line following ---
// Sensors are digital (0/1 per side), so the error signal is discrete:
// {-1, 0, +1} from (left - right). Same information content as
// bang-bang, but Kp/Ki/Kd shape a smoother, tunable reaction instead of
// an instant full-speed snap. All gains are untuned starting points.
static const int   BASE_SPEED     = 15;   // both wheels when centred (matches the bang-bang baseline that worked)
static const float KP             = 20.0f; // needs to reach ~BASE_SPEED at max error to turn as sharply as bang-bang did
static const float KI             = 0.0f;  // start at 0 - only add if there's a consistent one-sided drift
// static const float KD             = 0.1f;  // was jerky
static const float KD             = 0.0f;
                                            // was 2.0: with a discrete 0/1/-1 error and DT=0.02s, a full-step
                                            // transition gives derivative=1/DT=50, so Kd=2 alone contributed a
                                            // 100-point swing - far bigger than Kp's own ±20. This was the main
                                            // source of the jerkiness. Re-tune up from here in small steps if needed.
static const float INTEGRAL_LIMIT = 10.0f; // anti-windup clamp
static const int   LOOP_MS        = 20;
static const float DT             = LOOP_MS / 1000.0f;
static const int   MIN_SPEED      = -40;  // allow the inner wheel to reverse for a tighter pivot on sharp error
static const int   MAX_SPEED      = 100;
static const int   MAX_STEP       = 4;    // slew-rate limit: max change in commanded wheel speed per loop (smooths out any remaining sudden jumps)

static int clampSpeed(float v)
{
    if (v < MIN_SPEED) return MIN_SPEED;
    if (v > MAX_SPEED) return MAX_SPEED;
    return (int)v;
}

// Steps 'current' toward 'target' by at most MAX_STEP, so wheel commands
// change gradually loop-to-loop instead of jumping straight to the new
// PID output.
static int slewLimit(int current, int target)
{
    int delta = target - current;
    if (delta > MAX_STEP)  delta = MAX_STEP;
    if (delta < -MAX_STEP) delta = -MAX_STEP;
    return current + delta;
}

int main()
{
    uBit.init();
    uBit.radio.enable();
    uBit.radio.setGroup(1);

    float integral  = 0.0f;
    float lastError = 0.0f;
    int   currentLeftSpeed  = 0;
    int   currentRightSpeed = 0;

    while (1)
    {
        // Answer any diagnostic poll from base without blocking line
        // following - useful for watching sensor state on serial while
        // the robot is actually driving.
        uint8_t rx[32];
        int n = uBit.radio.datagram.recv(rx, 31);
        if (n > 0)
        {
            RobotCommand cmd;
            if (unpackRobotCommand(rx, n, cmd) &&
                (cmd.robotId == THIS_ROBOT_ID || cmd.robotId == 0) &&
                cmd.command == CMD_POLL_SENSORS)
            {
                bool leftBlack  = robot.trackSide(LineSide::Left, LineState::Black);
                bool rightBlack = robot.trackSide(LineSide::Right, LineState::Black);

                uint8_t tx[ROBOT_COMMAND_SIZE];
                int len = packRobotCommand(tx, THIS_ROBOT_ID, CMD_SENSOR_REPORT,
                                            leftBlack ? 1 : 0, rightBlack ? 1 : 0);
                uBit.radio.datagram.send(tx, len);
            }
        }

        // PID line following on a discrete {-1, 0, +1} error.
        // Turn convention (matches the bang-bang version that worked):
        // positive error/output -> steer left, negative -> steer right.
        bool leftBlack  = robot.trackSide(LineSide::Left, LineState::Black);
        bool rightBlack = robot.trackSide(LineSide::Right, LineState::Black);

        if (!leftBlack && !rightBlack)
        {
            // Line lost - stop rather than guess, same as the bang-bang
            // version. Reset the integral so it doesn't wind up while
            // stopped and cause a lurch when the line is reacquired.
            robot.stopCar();
            integral  = 0.0f;
            lastError = 0.0f;
            currentLeftSpeed  = 0;
            currentRightSpeed = 0;
        }
        else
        {
            float error = (leftBlack ? 1.0f : 0.0f) - (rightBlack ? 1.0f : 0.0f);

            integral += error * DT;
            if (integral > INTEGRAL_LIMIT)  integral = INTEGRAL_LIMIT;
            if (integral < -INTEGRAL_LIMIT) integral = -INTEGRAL_LIMIT;

            float derivative = (error - lastError) / DT;
            lastError = error;

            float output = (KP * error) + (KI * integral) + (KD * derivative);

            int targetLeftSpeed  = clampSpeed(BASE_SPEED - output);
            int targetRightSpeed = clampSpeed(BASE_SPEED + output);

            currentLeftSpeed  = slewLimit(currentLeftSpeed, targetLeftSpeed);
            currentRightSpeed = slewLimit(currentRightSpeed, targetRightSpeed);

            robot.setWheels(currentLeftSpeed, currentRightSpeed);
        }

        uBit.sleep(LOOP_MS);
    }
}
