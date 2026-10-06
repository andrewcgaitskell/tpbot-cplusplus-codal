#include "TPBotV1.h"

extern MicroBit uBit;

namespace TPBotV1 {

    static int mapRange(int value, int in_min, int in_max, int out_min, int out_max) {
        if (in_max == in_min) return out_min;
        long long mapped = static_cast<long long>(value - in_min) *
                           static_cast<long long>(out_max - out_min) /
                           static_cast<long long>(in_max - in_min);
        return static_cast<int>(mapped + out_min);
    }

    TPBotV1Driver::TPBotV1Driver() : _initEvents(false) {
        buff_[0] = 0;
        buff_[1] = 0;
        buff_[2] = 0;
        buff_[3] = 0;
    }

    void TPBotV1Driver::setWheels(int lspeed, int rspeed) {
        if (lspeed > 100) lspeed = 100;
        else if (lspeed < -100) lspeed = -100;

        if (rspeed > 100) rspeed = 100;
        else if (rspeed < -100) rspeed = -100;

        buff_[0] = 0x01;
        buff_[1] = (uint8_t)abs(lspeed);
        buff_[2] = (uint8_t)abs(rspeed);
        buff_[3] = 0;

        if (lspeed < 0 && rspeed < 0) {
            buff_[3] = 0x03;
        } else if (lspeed < 0) {
            buff_[3] = 0x01;
        } else if (rspeed < 0) {
            buff_[3] = 0x02;
        }

        send();
    }

    void TPBotV1Driver::setTravelTime(DriveDirection direction, int speed, int timeMs) {
        setTravelSpeed(direction, speed);
        uBit.sleep(timeMs);
        stopCar();
    }

    void TPBotV1Driver::setTravelSpeed(DriveDirection direction, int speed) {
        switch (direction) {
            case DriveDirection::Forward:
                setWheels(speed, speed);
                break;
            case DriveDirection::Backward:
                setWheels(-speed, -speed);
                break;
            case DriveDirection::Left:
                setWheels(-speed, speed);
                break;
            case DriveDirection::Right:
                setWheels(speed, -speed);
                break;
        }
    }

    void TPBotV1Driver::stopCar() {
        buff_[0] = 0x01;
        buff_[1] = 0;
        buff_[2] = 0;
        buff_[3] = 0;
        send();
    }

    bool TPBotV1Driver::trackSide(LineSide side, LineState state) const {
        uBit.io.P13.setPull(PullMode::None);
        uBit.io.P14.setPull(PullMode::None);

        int left_tracking = uBit.io.P13.getDigitalValue();
        int right_tracking = uBit.io.P14.getDigitalValue();

        if (side == LineSide::Left && state == LineState::White && left_tracking == 1) {
            return true;
        }
        if (side == LineSide::Left && state == LineState::Black && left_tracking == 0) {
            return true;
        }
        if (side == LineSide::Right && state == LineState::White && right_tracking == 1) {
            return true;
        }
        if (side == LineSide::Right && state == LineState::Black && right_tracking == 0) {
            return true;
        }

        return false;
    }

    bool TPBotV1Driver::trackLine(TrackingState state) const {
        uBit.io.P13.setPull(PullMode::None);
        uBit.io.P14.setPull(PullMode::None);

        int left_tracking = uBit.io.P13.getDigitalValue();
        int right_tracking = uBit.io.P14.getDigitalValue();

        if (left_tracking == 0 && right_tracking == 0 && state == TrackingState::L_R_line) {
            return true;
        }
        if (left_tracking == 1 && right_tracking == 0 && state == TrackingState::L_unline_R_line) {
            return true;
        }
        if (left_tracking == 0 && right_tracking == 1 && state == TrackingState::L_line_R_unline) {
            return true;
        }
        if (left_tracking == 1 && right_tracking == 1 && state == TrackingState::L_R_unline) {
            return true;
        }

        return false;
    }

    void TPBotV1Driver::trackEvent(MbPins side, MbEvents state, void (*handler)(MicroBitEvent)) {
        initEvents();
        uBit.messageBus.listen((int)side, (int)state, handler);
    }

    int TPBotV1Driver::sonarReturn(SonarUnit unit, int maxCmDistance) const {
        uBit.io.P16.setPull(PullMode::None);
        uBit.io.P16.setDigitalValue(0);
        system_timer_wait_us(2);

        uBit.io.P16.setDigitalValue(1);
        system_timer_wait_us(10);
        uBit.io.P16.setDigitalValue(0);

        uint32_t start = system_timer_current_time_us();

        while (uBit.io.P15.getDigitalValue() == 0) {
            if ((system_timer_current_time_us() - start) > (uint32_t)(maxCmDistance * 5800)) {
                return 0;
            }
        }

        uint32_t echo_start = system_timer_current_time_us();

        while (uBit.io.P15.getDigitalValue() == 1) {
            if ((system_timer_current_time_us() - echo_start) > (uint32_t)(maxCmDistance * 5800)) {
                return maxCmDistance;
            }
        }

        uint32_t elapsed_us = system_timer_current_time_us() - echo_start;
        int distance_cm = (int)(elapsed_us / 58);

        if (unit == SonarUnit::Inches) {
            return (int)(distance_cm * 0.393701f);
        }

        return distance_cm;
    }

    bool TPBotV1Driver::sonarJudge(SonarJudge judge, int dis) const {
        int measured = sonarReturn(SonarUnit::Centimeters);

        if (judge == SonarJudge::Less) {
            return (measured < dis && measured != 0);
        } else {
            return (measured > dis);
        }
    }

    void TPBotV1Driver::headlightColor(uint32_t color) {
        int r = (color >> 16) & 0xFF;
        int g = (color >> 8) & 0xFF;
        int b = color & 0xFF;
        headlightRGB(r, g, b);
    }

    void TPBotV1Driver::headlightRGB(int r, int g, int b) {
        buff_[0] = 0x20;
        buff_[1] = (uint8_t)r;
        buff_[2] = (uint8_t)g;
        buff_[3] = (uint8_t)b;
        send();
    }

    void TPBotV1Driver::headlightClose() {
        headlightRGB(0, 0, 0);
    }

    void TPBotV1Driver::setServo360(ServoList servo, int speed) {
        speed = mapRange(speed, -100, 100, 0, 180);

        switch (servo) {
            case ServoList::S1: buff_[0] = 0x10; break;
            case ServoList::S2: buff_[0] = 0x11; break;
            case ServoList::S3: buff_[0] = 0x12; break;
            case ServoList::S4: buff_[0] = 0x13; break;
        }

        buff_[1] = (uint8_t)speed;
        buff_[2] = 0;
        buff_[3] = 0;
        send();
    }

    void TPBotV1Driver::setServo(ServoTypeList servoType, ServoList servo, int angle) {
        switch (servo) {
            case ServoList::S1: buff_[0] = 0x10; break;
            case ServoList::S2: buff_[0] = 0x11; break;
            case ServoList::S3: buff_[0] = 0x12; break;
            case ServoList::S4: buff_[0] = 0x13; break;
        }

        switch (servoType) {
            case ServoTypeList::S180:
                angle = mapRange(angle, 0, 180, 0, 180);
                break;
            case ServoTypeList::S360:
                angle = mapRange(angle, 0, 360, 0, 180);
                break;
        }

        buff_[1] = (uint8_t)angle;
        buff_[2] = 0;
        buff_[3] = 0;
        send();
    }

    void TPBotV1Driver::send() {
        uBit.i2c.write(TPBotAdd, buff_, 4);
    }

    void TPBotV1Driver::initEvents() {
        if (!_initEvents) {
            uBit.io.P13.eventOn(MICROBIT_PIN_EVT_BOTH);
            uBit.io.P14.eventOn(MICROBIT_PIN_EVT_BOTH);
            _initEvents = true;
        }
    }

    TPBotV1Driver tpbot;
}
