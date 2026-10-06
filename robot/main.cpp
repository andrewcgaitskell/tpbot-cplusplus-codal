#include "MicroBit.h"
#include "TPBotMotor.h"

MicroBit uBit;

int main()
{
    uBit.init();
    uBit.sleep(500);

    uBit.serial.printf("\r\n--- TPBot V1 motor + line sensor test ---\r\n");
    uBit.serial.printf("Power-cycle TPBot BEFORE running this.\r\n\r\n");

    TPBotMotor motor(uBit);
    TPBotLineSensors lineSensors(uBit);

    uBit.serial.printf("Reading line sensors for 5 seconds "
                        "(try moving TPBot over a dark line)...\r\n");
    for (int i = 0; i < 10; i++)
    {
        TPBotLineState left = lineSensors.readLeft();
        TPBotLineState right = lineSensors.readRight();
        uBit.serial.printf("left=%s right=%s\r\n",
                            left == TPBotLineState::Black ? "Black" : "White",
                            right == TPBotLineState::Black ? "Black" : "White");
        uBit.sleep(500);
    }

    uBit.serial.printf("\r\nStarting motors (forward, speed 40)...\r\n");
    int startResult = motor.setWheels(40, 40);
    uBit.serial.printf("start result=%d %s\r\n",
                        startResult, startResult == DEVICE_OK ? "OK" : "FAILED");

    uBit.sleep(2000);

    uBit.serial.printf("Stopping motors...\r\n");
    int stopResult = motor.stop();
    uBit.serial.printf("stop result=%d %s\r\n",
                        stopResult, stopResult == DEVICE_OK ? "OK" : "FAILED");

    uBit.serial.printf("--- end ---\r\n");

    release_fiber();
}
