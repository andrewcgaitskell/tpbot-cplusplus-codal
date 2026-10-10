// main_smoothcircle_cpp_v2.txt

#include "MicroBit.h"
#include "TPBotV1.h"

MicroBit uBit;
using namespace TPBotV1;

static inline int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

int main() {
    uBit.init();

    const int BASE_SPEED = 28;   // both sensors on the line
    const int LOST_SPEED = 12;   // both sensors off the line
    const int SLEEP_MS   = 10;

    // Steering depends only on how long the current error has lasted
    const int STEER_MIN  = 4;    // gentle arc on first sign of drifting
    const int STEER_STEP = 2;    // extra steer per loop while still off-centre
    const int STEER_MAX  = 20;   // hard limit (tight enough for ~10 cm radius)
    const int SLEW       = 6;    // max wheel-speed change per loop

    const int SONAR_STOP_CM = 10, SONAR_MAX_CM = 20, SONAR_EVERY = 5;

    int lastE = 0, persist = 0;
    int curL = 0, curR = 0, tick = 0;
    bool blocked = false;

    while (1) {
        if (tick++ % SONAR_EVERY == 0) {
            int d = tpbot.sonarReturn(SonarUnit::Centimeters, SONAR_MAX_CM);
            blocked = (d > 0 && d < SONAR_STOP_CM);
        }
        if (blocked) {
            curL = curR = 0;
            tpbot.stopCar();
            uBit.sleep(SLEEP_MS);
            continue;
        }

        int l = uBit.io.P13.getDigitalValue();   // 0 = on line, 1 = off line
        int r = uBit.io.P14.getDigitalValue();

        int e = 0, base = BASE_SPEED, steer = 0;

        if (l == 0 && r == 0) {                  // centred: go straight, no memory
            persist = 0;
        } else if (l == 1 && r == 1) {           // lost: keep turning the way it was last drifting
            e = lastE;
            base = LOST_SPEED;
            steer = e * STEER_MAX;
        } else {                                 // one sensor off: correct, ramp while it persists
            e = (l == 1) ? +1 : -1;              // +1 = line is to the right
            persist = (e == lastE) ? persist + 1 : 1;
            lastE = e;
            int mag = clampi(STEER_MIN + persist * STEER_STEP, 0, STEER_MAX);
            steer = e * mag;
            base = BASE_SPEED - mag / 2;         // slow down as correction grows
        }

        // steer > 0 = turn right
        int targetL = clampi(base + steer, 0, 100);
        int targetR = clampi(base - steer, 0, 100);

        curL += clampi(targetL - curL, -SLEW, SLEW);
        curR += clampi(targetR - curR, -SLEW, SLEW);

        tpbot.setWheels(curL, curR);   // check the name/signature in TPBotV1.h
        uBit.sleep(SLEEP_MS);
    }
}
