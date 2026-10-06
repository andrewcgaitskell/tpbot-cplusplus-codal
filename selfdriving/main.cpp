#include "MicroBit.h"
#include "TPBotV1.h"

using namespace TPBotV1;

int main() {
    uBit.init();

    // If your line is black on white, keep the logic below.
    // If your line is white on black, invert the sensor logic.
    // uBit.display.scroll("TPBot V1");

    while (1) {
        int left = uBit.io.P13.getDigitalValue();   // 0 = black seen, 1 = white seen
        int right = uBit.io.P14.getDigitalValue();  // 0 = black seen, 1 = white seen

        // Both sensors detect the line: move forward
        if (left == 0 && right == 0) {
            tpbot.setTravelSpeed(DriveDirection::Forward, 45);
        }
        // Left sensor sees the line; turn left
        else if (left == 0 && right == 1) {
            tpbot.setTravelSpeed(DriveDirection::Left, 42);
        }
        // Right sensor sees the line; turn right
        else if (left == 1 && right == 0) {
            tpbot.setTravelSpeed(DriveDirection::Right, 42);
        }
        // Neither sensor sees the line: stop or slow turn
        else {
            tpbot.stopCar();
        }

        uBit.sleep(20);
    }
}
