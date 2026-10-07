#include "MicroBit.h"
#include "TPBotV1.h"

MicroBit uBit;
using namespace TPBotV1;

int main() {
    uBit.init();

    const int SAFE_DIST_CM = 12;
    const int FORWARD_SPEED = 40;
    const int TURN_SPEED = 60;
    const int REVERSE_SPEED = 25;

    while (1) {
        int distance = tpbot.sonarReturn(SonarUnit::Centimeters, 100);

        // Obstacle detection: stop, reverse, random dodge
        if (distance > 0 && distance < SAFE_DIST_CM) {
            tpbot.stopCar();
            tpbot.headlightRGB(255, 0, 0); // red alert

            tpbot.setTravelSpeed(DriveDirection::Backward, REVERSE_SPEED);
            uBit.sleep(200 + uBit.random(250));

            // Random dodge direction
            if (uBit.random(2) == 0) {
                tpbot.setTravelSpeed(DriveDirection::Left, TURN_SPEED);
            } else {
                tpbot.setTravelSpeed(DriveDirection::Right, TURN_SPEED);
            }
            uBit.sleep(350 + uBit.random(500));

            tpbot.stopCar();
            uBit.sleep(120);
            continue;
        }

        // Chaotic party dance
        int move = uBit.random(8);

        switch (move) {
            case 0:
                tpbot.headlightRGB(0, 255, 255);   // cyan
                tpbot.setTravelSpeed(DriveDirection::Forward, FORWARD_SPEED);
                break;

            case 1:
                tpbot.headlightRGB(255, 0, 255);   // magenta
                tpbot.setTravelSpeed(DriveDirection::Left, TURN_SPEED);
                break;

            case 2:
                tpbot.headlightRGB(255, 165, 0);   // orange
                tpbot.setTravelSpeed(DriveDirection::Right, TURN_SPEED);
                break;

            case 3:
                tpbot.headlightRGB(255, 255, 0);   // yellow
                tpbot.setTravelSpeed(DriveDirection::Forward, FORWARD_SPEED + 15);
                break;

            case 4:
                tpbot.headlightRGB(0, 255, 0);     // green
                tpbot.setTravelSpeed(DriveDirection::Left, TURN_SPEED + 10);
                break;

            case 5:
                tpbot.headlightRGB(150, 0, 255);   // purple
                tpbot.setTravelSpeed(DriveDirection::Right, TURN_SPEED + 10);
                break;

            case 6:
                tpbot.headlightRGB(255, 50, 120);  // pink
                tpbot.setTravelSpeed(DriveDirection::Forward, 20);
                uBit.sleep(80);
                tpbot.setTravelSpeed(DriveDirection::Left, TURN_SPEED + 20);
                break;

            default:
                tpbot.headlightRGB(255, 255, 255); // white flash
                tpbot.setTravelSpeed(DriveDirection::Right, TURN_SPEED + 15);
                break;
        }

        // Random timing for chaos
        uBit.sleep(80 + uBit.random(180));
    }
}
