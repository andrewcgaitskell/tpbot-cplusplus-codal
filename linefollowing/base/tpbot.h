#ifndef TPBOT_H
#define TPBOT_H

#include "MicroBit.h"

// C++ port of TPBot motor + line-sensor control, naming matched to
// elecfreaks/pxt-TPBot's own namespace TPBot (V1.ts / V2.ts) rather than
// invented names, so this reads consistently with the official extension
// and its docs. Confirmed working against a V1-protocol unit (plain
// 4-byte I2C frame, no command header).
//
// TPBot's I2C address is 0x10 (7-bit); CODAL's MicroBitI2C wants the
// 8-bit shifted form, confirmed working as 0x10 << 1 = 0x20.
//
// Motor frame: [0x01, |lspeed|, |rspeed|, direction]
//   direction: 0=none reversed, 1=left reversed, 2=right reversed, 3=both
//
// Line sensors are plain digital pins, not I2C: P13 = left, P14 = right.
// Pin value 0 = "Black" (on a dark line), 1 = "White" (off it) - matches
// the enum values used in the pxt source (Black=0, White=1).

enum class LineState
{
    Black = 0,
    White = 1
};

enum class LineSide
{
    Left = 0,
    Right = 1
};

// Matches TPBot.TrackingState in the pxt source exactly.
enum class TrackingState
{
    L_R_line = 0,
    L_unline_R_line = 1,
    L_line_R_unline = 2,
    L_R_unline = 3
};

class TPBot
{
public:
    explicit TPBot(MicroBit &ubit) : uBit(ubit)
    {
        uBit.io.P13.setPull(PullMode::None);
        uBit.io.P14.setPull(PullMode::None);
    }

    // Set the speed of left and right wheels. lspeed/rspeed: -100..100.
    // Negative reverses that wheel. Matches TPBot.setWheels() in pxt.
    // Returns the codal I2C write result (DEVICE_OK on success).
    int setWheels(int lspeed, int rspeed)
    {
        if (lspeed > 100) lspeed = 100;
        if (lspeed < -100) lspeed = -100;
        if (rspeed > 100) rspeed = 100;
        if (rspeed < -100) rspeed = -100;

        uint8_t direction = 0;
        if (lspeed < 0) direction |= 0x01;
        if (rspeed < 0) direction |= 0x02;

        int l = lspeed < 0 ? -lspeed : lspeed;
        int r = rspeed < 0 ? -rspeed : rspeed;

        uint8_t buff[4] = { 0x01, (uint8_t)l, (uint8_t)r, direction };
        return uBit.i2c.write(TPBOT_I2C_ADDR_8BIT, buff, 4, false);
    }

    // Stop the car. Matches TPBot.stopCar() in pxt.
    int stopCar()
    {
        return setWheels(0, 0);
    }

    // True if the given side currently reads the given line state.
    // Matches TPBot.trackSide() in pxt.
    bool trackSide(LineSide side, LineState state)
    {
        LineState actual = (side == LineSide::Left) ? readLeft() : readRight();
        return actual == state;
    }

    // True if the combined left/right sensor reading matches the given
    // tracking state. Matches TPBot.trackLine() in pxt.
    bool trackLine(TrackingState state)
    {
        return currentTrackingState() == state;
    }

    // Not part of pxt's block API (pxt only exposes the boolean testers
    // above), but useful in C++ where you often want the raw state
    // directly rather than testing against one value at a time.
    TrackingState currentTrackingState()
    {
        bool left = readLeft() == LineState::Black;
        bool right = readRight() == LineState::Black;

        if (left && right)   return TrackingState::L_R_line;
        if (!left && right)  return TrackingState::L_unline_R_line;
        if (left && !right)  return TrackingState::L_line_R_unline;
        return TrackingState::L_R_unline;
    }

private:
    MicroBit &uBit;
    static const uint16_t TPBOT_I2C_ADDR_8BIT = 0x10 << 1;  // = 0x20

    LineState readLeft()
    {
        return uBit.io.P13.getDigitalValue() ? LineState::White : LineState::Black;
    }

    LineState readRight()
    {
        return uBit.io.P14.getDigitalValue() ? LineState::White : LineState::Black;
    }
};

#endif
