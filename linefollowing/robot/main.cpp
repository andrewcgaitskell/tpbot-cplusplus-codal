#include "MicroBit.h"
#include "tpbot.h"

MicroBit uBit;
TPBot robot(uBit);

// Kept for consistency with the base's message format, though with no
// joystick control there's only ever one robot to identify.
static const uint8_t THIS_ROBOT_ID = 1;

static const int SEND_INTERVAL_MS = 50;

int main()
{
    uBit.init();
    uBit.radio.enable();
    uBit.radio.setGroup(1);

    while (1)
    {
        bool leftBlack  = robot.trackSide(LineSide::Left, LineState::Black);
        bool rightBlack = robot.trackSide(LineSide::Right, LineState::Black);

        // robotId,left,right  (left/right: 1 = on line/black, 0 = off/white)
        ManagedString msg = ManagedString(THIS_ROBOT_ID) + ","
                           + ManagedString(leftBlack ? 1 : 0) + ","
                           + ManagedString(rightBlack ? 1 : 0);

        uBit.radio.datagram.send((uint8_t *)msg.toCharArray(), msg.length());

        uBit.sleep(SEND_INTERVAL_MS);
    }
}
