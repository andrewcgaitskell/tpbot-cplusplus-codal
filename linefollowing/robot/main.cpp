#include "MicroBit.h"
#include "tpbot.h"
#include "tpbotprotocol.h"

MicroBit uBit;
TPBot robot(uBit);

// This robot's ID. Change per-robot if you run more than one on the
// same radio group. RobotID 0 in an incoming packet is treated as
// "broadcast to all robots" and is always acted on.

static const uint8_t THIS_ROBOT_ID = 1;

// --- Bang-bang line following ---
// Deliberately slow and simple: no PID, no speed ramping, no search
// behaviour if the line is lost. Just react to the current sensor state
// each loop. Tune these once basic tracking direction is confirmed.
static const int FORWARD_SPEED = 10;   // both wheels, when centred on the line
static const int TURN_SPEED    = 10;   // the still-tracking wheel, while the other stops

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

        // Bang-bang line following. Turn direction swapped from the first
        // attempt - it was correcting the wrong way and running straight
        // off the line into L_R_unline (stop) almost immediately.
        switch (robot.currentTrackingState())
        {
            case TrackingState::L_R_line:
                // Both sensors see the line - centred, go straight.
                robot.setWheels(FORWARD_SPEED, FORWARD_SPEED);
                break;

            case TrackingState::L_line_R_unline:
                // Left still on the line, right has drifted off - steer left.
                robot.setWheels(0, TURN_SPEED);
                break;

            case TrackingState::L_unline_R_line:
                // Right still on the line, left has drifted off - steer right.
                robot.setWheels(TURN_SPEED, 0);
                break;

            case TrackingState::L_R_unline:
            default:
                // Line lost - stop rather than guess, for now.
                robot.stopCar();
                break;
        }

        uBit.sleep(20);
    }
}
