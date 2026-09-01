#include "MicroBit.h"
#include "tpbot.h"

MicroBit uBit;
TPBot robot(uBit);

// Diagnostic only - NOT the line follower. Drives both wheels at the
// same commanded speed with no sensor input at all, to check for a raw
// mechanical/motor imbalance. Place the robot on a flat surface (off
// any line) and watch whether it drives dead straight or visibly arcs
// to one side. If it arcs, that's a hardware imbalance the line-following
// logic has to correct for even when centred - which would explain why
// it "drives off almost immediately" on a straight despite the code
// being unchanged since the peanut track worked.

static const int TEST_SPEED = 15; // matches BASE_SPEED in the line follower

int main()
{
    uBit.init();

    uBit.display.scroll("STRAIGHT TEST");

    robot.setWheels(TEST_SPEED, TEST_SPEED);

    while (1)
    {
        uBit.sleep(1000);
    }
}
