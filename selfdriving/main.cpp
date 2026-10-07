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
        const int BASE_FORWARD   = 30;
        const int TURN_SPEED_L   = 30;   // left turn speed
        const int TURN_SPEED_R   = 30;   // right turn speed

        // other
        const int RECOVERY_SPEED = 10;   // speed when both sensors lose line
        const int SLEEP_TIME     = 20;

        // Smoother tracking: proportional correction based on sensor state
        if (left == 0 && right == 0) {
            // Both sensors see black: go forward
            tpbot.setTravelSpeed(DriveDirection::Forward, BASE_FORWARD);
        }
        else if (left == 0 && right == 1) {
            // Left sees black, right sees white: turn left
            tpbot.setTravelSpeed(DriveDirection::Left, TURN_SPEED_L);
        }
        else if (left == 1 && right == 0) {
            // Right sees black, left sees white: turn right
            tpbot.setTravelSpeed(DriveDirection::Right, TURN_SPEED_R);
        }
        else {
            // Both sensors lost the line: slow recovery
            tpbot.setTravelSpeed(DriveDirection::Forward, RECOVERY_SPEED);
        }

        uBit.sleep(SLEEP_TIME);
    }
}

