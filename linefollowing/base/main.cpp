#include "MicroBit.h"
#include "tpbot.h"
#include "tpbotprotocol.h"

MicroBit uBit;

static const int POLL_INTERVAL_MS = 50;
static const uint8_t TARGET_ROBOT_ID = 1;

const char *lineStateToString(LineState state)
{
    return state == LineState::Black ? "black" : "white";
}

int main()
{
    uBit.init();
    uBit.serial.setBaud(115200);
    uBit.radio.enable();
    uBit.radio.setGroup(1);

    uBit.serial.printf("# base station up, group=1\r\n");

    // Clear stale radio packets.
    int cleared = 0;
    for (int i = 0; i < 10; i++)
    {
        uint8_t junk[32];
        int n = uBit.radio.datagram.recv(junk, 31);
        if (n < 0)
            break;
        cleared++;
    }
    uBit.serial.printf("# cleared %d stale packet(s)\r\n", cleared);

    while (1)
    {
        uint8_t tx[ROBOT_COMMAND_SIZE];
        int len = packRobotCommand(tx, TARGET_ROBOT_ID, CMD_POLL_SENSORS, 0, 0);
        uBit.radio.datagram.send(tx, len);

        // Wait for the reply, bounded to ~100ms.
        uint8_t rx[32];
        int received = -1;
        int waited = 0;
        for (int i = 0; i < 100; i++)
        {
            received = uBit.radio.datagram.recv(rx, 31);
            if (received >= 0)
                break;
            uBit.sleep(1);
            waited++;
        }

        if (received > 0)
        {
            RobotCommand reply;
            if (unpackRobotCommand(rx, received, reply) && reply.command == CMD_SENSOR_REPORT)
            {
                LineState left  = reply.value1 ? LineState::Black : LineState::White;
                LineState right = reply.value2 ? LineState::Black : LineState::White;

                bool leftBlack  = left  == LineState::Black;
                bool rightBlack = right == LineState::Black;

                TrackingState state;
                if (leftBlack && rightBlack)        state = TrackingState::L_R_line;
                else if (!leftBlack && rightBlack)  state = TrackingState::L_unline_R_line;
                else if (leftBlack && !rightBlack)  state = TrackingState::L_line_R_unline;
                else                                 state = TrackingState::L_R_unline;

                uBit.serial.printf("robot=%d left=%s right=%s state=%d\r\n",
                                    reply.robotId,
                                    lineStateToString(left),
                                    lineStateToString(right),
                                    (int)state);
            }
            else
            {
                uBit.serial.printf("# unexpected reply, len=%d\r\n", received);
            }
        }
        else
        {
            uBit.serial.printf("# no reply after %dms\r\n", waited);
        }

        uBit.sleep(POLL_INTERVAL_MS);
    }
}
