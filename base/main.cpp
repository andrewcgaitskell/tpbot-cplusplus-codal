#include "MicroBit.h"
#include "tpbotprotocol.h"

MicroBit uBit;

int stringToInt(const char *str)
{
    int result = 0;
    bool negative = false;
    if (*str == '-') { negative = true; str++; }
    while (*str >= '0' && *str <= '9')
    {
        result = result * 10 + (*str - '0');
        str++;
    }
    return negative ? -result : result;
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
        // Expect a line like: "1,1,40,40"  ->  robotId,command,value1,value2
        ManagedString msg = uBit.serial.readUntil("\n");
        if (msg.length() == 0)
            continue;
        if (msg.charAt(msg.length() - 1) == '\r')
            msg = msg.substring(0, msg.length() - 1);

        // Split into up to 4 comma-separated fields.
        int fields[4] = { 0, 0, 0, 0 };
        int fieldIndex = 0;
        int fieldStart = 0;
        for (int i = 0; i <= msg.length() && fieldIndex < 4; i++)
        {
            if (i == msg.length() || msg.charAt(i) == ',')
            {
                ManagedString piece = msg.substring(fieldStart, i - fieldStart);
                fields[fieldIndex] = stringToInt(piece.toCharArray());
                fieldIndex++;
                fieldStart = i + 1;
            }
        }

        if (fieldIndex < 4)
        {
            uBit.serial.printf("# malformed line, expected 4 fields, got %d\r\n", fieldIndex);
            continue;
        }

        uint8_t robotId = (uint8_t)fields[0];
        uint8_t command  = (uint8_t)fields[1];
        int16_t value1   = (int16_t)fields[2];
        int16_t value2   = (int16_t)fields[3];

        uBit.serial.printf("# tx robotId=%d command=%d v1=%d v2=%d\r\n",
                            robotId, command, value1, value2);

        // Python -> Base -> Robot (binary)
        uint8_t tx[ROBOT_COMMAND_SIZE];
        int len = packRobotCommand(tx, robotId, command, value1, value2);
        uBit.radio.datagram.send(tx, len);

        // Wait for Robot -> Base.
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
            uBit.serial.printf("# rx len=%d after %dms\r\n", received, waited);
            // Base -> Python (robot's reply is still plain text, e.g. tracking state)
            uBit.serial.send(rx, received);
            uBit.serial.send("\n");
        }
        else
        {
            uBit.serial.printf("# no reply after %dms\r\n", waited);
        }
    }
}