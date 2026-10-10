//main_smoothwithstop_cpp.txt

#include "MicroBit.h"
#include "TPBotV1.h"

MicroBit uBit;
using namespace TPBotV1;

enum class Direction { Forward, Left, Right };

int main() {
    uBit.init();

    const int BASE_FORWARD = 30, TURN_SPEED_L = 30, TURN_SPEED_R = 30;
    const int RECOVERY_SPEED = 10, SLEEP_TIME = 20;
    const int SONAR_THRESHOLD_CM = 10;  // Stop if object closer than 10cm

    Direction lastGoodDirection = Direction::Forward;
    int speed = BASE_FORWARD;

    while (1) {
        int left = uBit.io.P13.getDigitalValue();
        int right = uBit.io.P14.getDigitalValue();

        // Check sonar for obstacles
        int distanceCm = tpbot.sonarReturn(SonarUnit::Centimeters, 50);
        if (distanceCm < SONAR_THRESHOLD_CM && distanceCm > 0) {
            // Object detected: stop
            tpbot.stopCar();
            uBit.sleep(SLEEP_TIME);
            continue;
        }

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
            direction = lastGoodDirection;
            speed = RECOVERY_SPEED;
        }

        if (left != 1 || right != 1) {
            lastGoodDirection = direction;
        }

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
