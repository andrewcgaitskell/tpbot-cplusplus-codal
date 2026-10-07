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

    const int BASE_FORWARD = 30;
    const int TURN_SPEED_L = 30;
    const int TURN_SPEED_R = 30;
    const int RECOVERY_SPEED = 10;
    const int SLEEP_TIME = 20;

    Direction lastGoodDirection = Direction::Forward;
    int speed = BASE_FORWARD;

    while (1) {
        int left  = uBit.io.P13.getDigitalValue();
        int right = uBit.io.P14.getDigitalValue();

        // Determine direction and update last good state
        Direction direction = Direction::Forward;
        if (left == 0 && right == 0) {
            direction = Direction::Forward;
            speed = BASE_FORWARD;
        }
        else if (left == 0 && right == 1) {
            direction = Direction::Left;
            speed = TURN_SPEED_L;
        }
        else if (left == 1 && right == 0) {
            direction = Direction::Right;
            speed = TURN_SPEED_R;
        }
        else {
            // Both sensors lost line: use last good direction
            direction = lastGoodDirection;
            speed = RECOVERY_SPEED;
        }

        // Only update lastGoodDirection on clear line sensor input
        if (left != 1 || right != 1) {
            lastGoodDirection = direction;
        }

        // Apply direction
        switch (direction) {
            case Direction::Forward:
                tpbot.setTravelSpeed(DriveDirection::Forward, speed);
                break;
            case Direction::Left:
                tpbot.setTravelSpeed(DriveDirection::Left, speed);
                break;
            case Direction::Right:
                tpbot.setTravelSpeed(DriveDirection::Right, speed);
                break;
        }

        uBit.sleep(SLEEP_TIME);
    }
}
