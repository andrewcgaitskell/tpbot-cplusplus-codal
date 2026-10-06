#include "MicroBit.h"
#include "TPBotV1.h"

MicroBit uBit;

using namespace TPBotV1;

int main() {
    uBit.init();
    uBit.display.scroll("MOTOR TEST");

    // Simple motor sanity test: run each direction for 1 second.
    // If the wheels move, the TPBot protocol is working.
    tpbot.setTravelSpeed(DriveDirection::Forward, 100);
    uBit.sleep(1000);

    tpbot.setTravelSpeed(DriveDirection::Backward, 100);
    uBit.sleep(1000);

    tpbot.setTravelSpeed(DriveDirection::Left, 100);
    uBit.sleep(1000);

    tpbot.setTravelSpeed(DriveDirection::Right, 100);
    uBit.sleep(1000);

    tpbot.stopCar();
    uBit.display.scroll("DONE");

    while (1) {
        uBit.sleep(100);
    }
}
