// main_smoothcircle_cpp.txt

#include "MicroBit.h"
#include "TPBotV1.h"

MicroBit uBit;
using namespace TPBotV1;

static inline int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

int main() {
    uBit.init();

    // Drive
    const int BASE_SPEED = 25;   // both-on-line speed
    const int LOST_SPEED = 12;   // speed when both sensors read 1
    const int SLEEP_MS   = 20;

    // Steering (tune in this order: BASE_SPEED, KP, KI/BIAS_MAX, SLEW)
    const int KP        = 6;     // immediate correction per sensor error
    const int KI        = 1;     // how fast the learned curvature builds per loop
    const int BIAS_MAX  = 10;    // cap on learned curvature
    const int STEER_MAX = 15;    // cap on total steer
    const int SLEW      = 5;     // max wheel-speed change per loop

    // Sonar
    const int SONAR_STOP_CM = 10, SONAR_MAX_CM = 20, SONAR_EVERY = 5;

    int bias = 0;        // learned steady-state curvature (+ = right)
    int lastE = 0;       // last non-zero error, used when the line is lost
    int curL = 0, curR = 0;
    int tick = 0;
    bool blocked = false;

    while (1) {
        // Poll sonar every 5th loop only: it blocks while waiting for the echo
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

        int l = uBit.io.P13.getDigitalValue();
        int r = uBit.io.P14.getDigitalValue();

        int e, base = BASE_SPEED;
        bool lost = false;
        if      (l == 0 && r == 0) e = 0;
        else if (l == 0 && r == 1) e = -1;   // left
        else if (l == 1 && r == 0) e = +1;   // right
        else { e = lastE; base = LOST_SPEED; lost = true; }

        if (!lost) {
            if (e != 0) {
                lastE = e;
                bias = clampi(bias + KI * e, -BIAS_MAX, BIAS_MAX);
            } else {
                bias = bias * 15 / 16;       // leaky: forgets on straights
            }
        }

        int steer = clampi(KP * e + bias, -STEER_MAX, STEER_MAX);

        // steer > 0 = turn right: left wheel faster, right wheel slower
        int targetL = clampi(base + steer, 0, 100);
        int targetR = clampi(base - steer, 0, 100);

        // Slew limit to smooth the transitions
        curL += clampi(targetL - curL, -SLEW, SLEW);
        curR += clampi(targetR - curR, -SLEW, SLEW);

        tpbot.setWheels(curL, curR);   // <-- check the name/signature in TPBotV1.h
        uBit.sleep(SLEEP_MS);
    }
}
