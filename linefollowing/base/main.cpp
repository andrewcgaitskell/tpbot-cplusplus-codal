#include "MicroBit.h"
#include "tpbot.h"
#include "tpbotprotocol.h"

MicroBit uBit;

static const int POLL_INTERVAL_MS = 50;
static const uint8_t TARGET_ROBOT_ID = 1;
static const int LOOP_TIME_POLL_EVERY_N = 5; // ask for loop timing once every N sensor polls

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

    int cycle = 0;

    while (1)
    {
        bool pollLoopTime = (cycle % LOOP_TIME_POLL_EVERY_N) == 0;
        uint8_t pollCommand = pollLoopTime ? CMD_POLL_LOOP_TIME : CMD_POLL_SENSORS;

        uint8_t tx[ROBOT_COMMAND_SIZE];
        int len = packRobotCommand(tx, TARGET_ROBOT_ID, pollCommand, 0, 0);
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
            if (!unpackRobotCommand(rx, received, reply))
            {
                uBit.serial.printf("# unexpected reply, len=%d\r\n", received);
            }
            else if (reply.command == CMD_SENSOR_REPORT)
            {
                LineState left = reply.value1 ? LineState::Black : LineState::White;
                LineState right = reply.value2 ? LineState::Black : LineState::White;
                bool leftBlack = left == LineState::Black;
                bool rightBlack = right == LineState::Black;

                TrackingState state;
                if (leftBlack && rightBlack) state = TrackingState::L_R_line;
                else if (!leftBlack && rightBlack) state = TrackingState::L_unline_R_line;
                else if (leftBlack && !rightBlack) state = TrackingState::L_line_R_unline;
                else state = TrackingState::L_R_unline;

                uBit.serial.printf("robot=%d left=%s right=%s state=%d\r\n",
                                    reply.robotId,
                                    lineStateToString(left),
                                    lineStateToString(right),
                                    (int)state);
            }
            else if (reply.command == CMD_LOOP_TIME_REPORT)
            {
                uBit.serial.printf("robot=%d loop_ms=%d max_loop_ms=%d\r\n",
                                    reply.robotId, reply.value1, reply.value2);
            }
            else
            {
                uBit.serial.printf("# unexpected reply command=%d\r\n", reply.command);
            }
        }
        else
        {
            uBit.serial.printf("# no reply after %dms\r\n", waited);
        }

        cycle++;
        uBit.sleep(POLL_INTERVAL_MS);
    }
}

