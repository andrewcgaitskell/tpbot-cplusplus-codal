#include "MicroBit.h"
#include "tpbot.h"

MicroBit uBit;
TPBot robot(uBit);

// Diagnostic only - NOT the line follower. Drives both wheels at a
// speed plus/minus TRIM, no sensor input at all, to find the constant
// offset needed to cancel a mechanical drift (confirmed: drifts left at
// equal wheel speeds). Place the robot on a flat surface (off any line).
// Start at TRIM=0 (confirms drift), then increase in small steps and
// re-test until it drives as straight as possible.

static const int TEST_SPEED = 15; // matches BASE_SPEED in the line follower
static const int TRIM       = 2;  // left gets +TRIM, right gets -TRIM - adjust and re-test

int main()
{
    uBit.init();

    uBit.display.scroll("STRAIGHT TEST");

    robot.setWheels(TEST_SPEED + TRIM, TEST_SPEED - TRIM);

    while (1)
    {
        uBit.sleep(1000);
    }
}
