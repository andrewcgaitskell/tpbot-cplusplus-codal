#include "MicroBit.h"
#include "TPBotV1.h"

MicroBit uBit;
using namespace TPBotV1;

int main() {
    uBit.init();

    const int SAFE_DIST_CM = 12;
    const int DANCE_STEP_MS = 180;
    const int FORWARD_SPEED = 35;
    const int TURN_SPEED = 45;
    const int REVERSE_SPEED = 20;

    while (1) {
        int distance = tpbot.sonarReturn(SonarUnit::Centimeters, 100);

        // Obstacle avoidance: stop and reverse away
        if (distance > 0 && distance < SAFE_DIST_CM) {
            tpbot.stopCar();
            tpbot.headlightRGB(255, 0, 0);  // red alert
            tpbot.setTravelSpeed(DriveDirection::Backward, REVERSE_SPEED);
            uBit.sleep(220);
            tpbot.setTravelSpeed(DriveDirection::Left, TURN_SPEED);
            uBit.sleep(500);
            tpbot.stopCar();
            uBit.sleep(150);
            continue;
        }

        // A dervish dance: forward + spin + whiplash turns
        uint32_t t = uBit.systemTime();
        int phase = (t / DANCE_STEP_MS) % 6;

        switch (phase) {
            case 0:
            case 3:
                tpbot.headlightRGB(0, 255, 255);  // cyan
                tpbot.setTravelSpeed(DriveDirection::Forward, FORWARD_SPEED);
                break;
            case 1:
            case 4:
                tpbot.headlightRGB(255, 0, 255);  // magenta
                tpbot.setTravelSpeed(DriveDirection::Left, TURN_SPEED);
                break;
            case 2:
            case 5:
                tpbot.headlightRGB(255, 165, 0);  // orange
                tpbot.setTravelSpeed(DriveDirection::Right, TURN_SPEED);
                break;
        }

        uBit.sleep(DANCE_STEP_MS);
    }
}
