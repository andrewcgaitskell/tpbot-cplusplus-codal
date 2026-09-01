#include "MicroBit.h"
#include "tpbot.h"
#include "tpbotprotocol.h"

MicroBit uBit;
TPBot robot(uBit);

// This robot's ID. Change per-robot if you run more than one on the
// same radio group. RobotID 0 in an incoming packet is treated as
// "broadcast to all robots" and is always acted on.

static const uint8_t THIS_ROBOT_ID = 1;

int main()
{
    uBit.init();
    uBit.radio.enable();
    uBit.radio.setGroup(1);

    while (1)
    {
        uint8_t rx[32];
        int n = uBit.radio.datagram.recv(rx, 31);
        if (n > 0)
        {
            RobotCommand cmd;
            if (unpackRobotCommand(rx, n, cmd))
            {
                if ((cmd.robotId == THIS_ROBOT_ID || cmd.robotId == 0) &&
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
        }
        uBit.sleep(1);
    }
}
