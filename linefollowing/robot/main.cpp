#include "MicroBit.h"
#include "tpbot.h"
#include "tpbotprotocol.h"

MicroBit uBit;
TPBot robot(uBit);

// This robot's ID. Change per-robot if you run more than one on the
// same radio group. RobotID 0 in an incoming packet is treated as
// "broadcast to all robots" and is always acted on.

static const uint8_t THIS_ROBOT_ID = 1;

// --- P-only line following ---
// Stripped back to a single variable (Kp) to tune in isolation. No Ki,
// no Kd, no debounce, no slew-rate limiting - those all add interacting
// parameters, which made it hard to tell what was actually causing
// undercorrection vs. zigzag. Get one working Kp first, then reintroduce
// extras one at a time only if a specific symptom calls for it.
//
// Error is discrete: {-1, 0, +1} from (left - right), since both
// sensors are digital. History so far: Kp=8 undercorrected (drifted off
// on corners), Kp=20 overcorrected (zigzag). Try this value, then adjust
// in steps of ~2-3 toward whichever symptom reappears.
static const int   BASE_SPEED = 15;
static const float KP         = 12.0f;
static const int   MIN_SPEED  = -25;  // allows the inner wheel to reverse for a tighter pivot on sharp error
static const int   MAX_SPEED  = 100;

static int clampSpeed(float v)
{
    if (v < MIN_SPEED) return MIN_SPEED;
    if (v > MAX_SPEED) return MAX_SPEED;
    return (int)v;
}

int main()
{
    uBit.init();
    uBit.radio.enable();
    uBit.radio.setGroup(1);

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

        // P-only line following on a discrete {-1, 0, +1} error.
        // Turn convention (matches the bang-bang version that worked):
        // positive error/output -> steer left, negative -> steer right.
        bool leftBlack  = robot.trackSide(LineSide::Left, LineState::Black);
        bool rightBlack = robot.trackSide(LineSide::Right, LineState::Black);

        if (!leftBlack && !rightBlack)
        {
            // Line lost - stop rather than guess, same as the bang-bang
            // version.
            robot.stopCar();
        }
        else
        {
            float error = (leftBlack ? 1.0f : 0.0f) - (rightBlack ? 1.0f : 0.0f);
            float output = KP * error;

            int leftSpeed  = clampSpeed(BASE_SPEED - output);
            int rightSpeed = clampSpeed(BASE_SPEED + output);

            robot.setWheels(leftSpeed, rightSpeed);
        }

        uBit.sleep(20);
    }
}
