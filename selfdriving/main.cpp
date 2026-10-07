#include "MicroBit.h"
#include "TPBotV1.h"

MicroBit uBit;

using namespace TPBotV1;

int main() {
    uBit.init();

    while (1) {
        int left  = uBit.io.P13.getDigitalValue();   // 0 = black seen, 1 = white seen
        int right = uBit.io.P14.getDigitalValue();   // 0 = black seen, 1 = white seen

        // tune these first
        const int BASE_FORWARD = 30;
        const int TURN_SPEED   = 25;

        // other
        const int SLEEP_TIME   = 20;

        if (left == 0 && right == 0) {
            tpbot.setTravelSpeed(DriveDirection::Forward, BASE_FORWARD);
        }
        else if (left == 0 && right == 1) {
            tpbot.setTravelSpeed(DriveDirection::Left, TURN_SPEED);
        }
        else if (left == 1 && right == 0) {
            tpbot.setTravelSpeed(DriveDirection::Right, TURN_SPEED);
        }
        else {
            // both sensors lost the line: slow recovery
            tpbot.setTravelSpeed(DriveDirection::Forward, 10);
        }

        uBit.sleep(SLEEP_TIME);
    }
}
