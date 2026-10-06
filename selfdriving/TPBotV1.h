#ifndef TPBOTV1_H
#define TPBOTV1_H

#include "MicroBit.h"
#include <cstdint>

namespace TPBotV1 {

    enum class DriveDirection {
        Forward = 0,
        Backward = 1,
        Left = 2,
        Right = 3
    };

    enum class TrackingState {
        L_R_line = 0,
        L_unline_R_line = 1,
        L_line_R_unline = 2,
        L_R_unline = 3
    };

    enum class SonarUnit {
        Centimeters = 0,
        Inches = 1
    };

    enum class SonarJudge {
        Less = 0,
        Greater = 1
    };

    enum class ServoList {
        S1 = 0,
        S2 = 1,
        S3 = 2,
        S4 = 3
    };

    enum class LineState {
        Black = 0,
        White = 1
    };

    enum class LineSide {
        Left = 0,
        Right = 1
    };

    enum class MbEvents {
        Black = MICROBIT_PIN_EVT_FALL,
        White = MICROBIT_PIN_EVT_RISE
    };

    enum class MbPins {
        Left = MICROBIT_ID_IO_P13,
        Right = MICROBIT_ID_IO_P14
    };

    enum class ServoTypeList {
        S180 = 0,
        S360 = 1
    };

    class TPBotV1Driver {
    public:
        static constexpr uint8_t TPBotAdd = 0x10 << 1;  // ✅ CORRECT (0x20)

        TPBotV1Driver();

        void setWheels(int lspeed = 50, int rspeed = 50);
        void setTravelTime(DriveDirection direction, int speed, int timeMs);
        void setTravelSpeed(DriveDirection direction, int speed);
        void stopCar();
        [[nodiscard]] bool trackSide(LineSide side, LineState state) const;
        [[nodiscard]] bool trackLine(TrackingState state) const;
        void trackEvent(MbPins side, MbEvents state, void (*handler)(MicroBitEvent));
        [[nodiscard]] int sonarReturn(SonarUnit unit, int maxCmDistance = 500) const;
        [[nodiscard]] bool sonarJudge(SonarJudge judge, int dis) const;
        void headlightColor(uint32_t color);
        void headlightRGB(int r, int g, int b);
        void headlightClose();
        void setServo360(ServoList servo, int speed = 100);
        void setServo(ServoTypeList servoType, ServoList servo, int angle = 0);

    private:
        uint8_t buff_[4];
        bool _initEvents;

        void send();
        void initEvents();
    };

    extern TPBotV1Driver tpbot;
}

#endif
