#include "MicroBit.h"
#include "TPBotV1.h"

MicroBit uBit;
using namespace TPBotV1;

enum class Direction {
    Forward,
    Left,
    Right
};

int main() {
    uBit.init();

    // tune these first
    const int BASE_FORWARD = 30;
    const int TURN_SPEED_L = 30; // left turn speed
    const int TURN_SPEED_R = 30; // right turn speed

    // other
    const int RECOVERY_SPEED = 10; // speed when both sensors lose line
    const int SLEEP_TIME = 20;

    Direction lastGoodDirection = Direction::Forward;

    while (1) {
        int left  = uBit.io.P13.getDigitalValue();   // 0 = black seen, 1 = white seen
        int right = uBit.io.P14.getDigitalValue();   // 0 = black seen, 1 = white seen

        if (left == 0 && right == 0) {
            // Both sensors see black: go forward
            lastGoodDirection = Direction::Forward;
            tpbot.setTravelSpeed(DriveDirection::Forward, BASE_FORWARD);
        }
        else if (left == 0 && right == 1) {
            // Left sees black, right sees white: turn left
            lastGoodDirection = Direction::Left;
            tpbot.setTravelSpeed(DriveDirection::Left, TURN_SPEED_L);
        }
        else if (left == 1 && right == 0) {
            // Right sees black, left sees white: turn right
            lastGoodDirection = Direction::Right;
            tpbot.setTravelSpeed(DriveDirection::Right, TURN_SPEED_R);
        }
        else {
            // Both sensors lost the line: recover using last good direction
            switch (lastGoodDirection) {
                case Direction::Forward:
                    tpbot.setTravelSpeed(DriveDirection::Forward, RECOVERY_SPEED);
                    break;
                case Direction::Left:
                    tpbot.setTravelSpeed(DriveDirection::Left, TURN_SPEED_L);
                    break;
                case Direction::Right:
                    tpbot.setTravelSpeed(DriveDirection::Right, TURN_SPEED_R);
                    break;
            }
        }

        uBit.sleep(SLEEP_TIME);
    }
}
