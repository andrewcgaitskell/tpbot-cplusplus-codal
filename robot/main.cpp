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
                if (cmd.robotId == THIS_ROBOT_ID || cmd.robotId == 0)
                {
                    switch (cmd.command)
                    {
                        case CMD_SET_WHEELS:
                            robot.setWheels(cmd.value1, cmd.value2);
                            break;
                        // Add more command cases here as the protocol grows.
                    }
                }
            }

            ManagedString reply((int)robot.currentTrackingState());
            uBit.radio.datagram.send((uint8_t *)reply.toCharArray(), reply.length());
        }
        uBit.sleep(1);
    }
}
