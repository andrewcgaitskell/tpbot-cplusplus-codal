#ifndef TPBOT_MOTOR_H
#define TPBOT_MOTOR_H

#include "MicroBit.h"

// C++ port of the TPBot motor and line-sensor control from
// elecfreaks/pxt-TPBot. This unit's onboard firmware was confirmed (by
// testing) to only respond to the V1 protocol, so motor control here
// sends only that format rather than hedging with both.
//
// TPBot's I2C address is 0x10 (7-bit); CODAL's MicroBitI2C wants the
// 8-bit shifted form, confirmed working as 0x10 << 1 = 0x20.
//
// V1 motor frame (confirmed working): a plain 4-byte write, no header:
//   [0x01, |lspeed|, |rspeed|, direction]
//   direction: 0=none reversed, 1=left reversed, 2=right reversed, 3=both
//
// Line sensors are NOT read over I2C at all - both V1.ts and V2.ts read
// them as plain digital pins: P13 = left sensor, P14 = right sensor.
// Pin reads directly (0 = "Black"/on-line, 1 = "White"/off-line, per the
// pxt source's own enum values), so this part of the library is
// unaffected by the V1/V2 protocol difference.

class TPBotMotor
{
public:
    explicit TPBotMotor(MicroBit &ubit) : uBit(ubit) {}

    // lspeed/rspeed: -100..100. Negative reverses that wheel.
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

    int stop()
    {
        return setWheels(0, 0);
    }

private:
    MicroBit &uBit;
    static const uint16_t TPBOT_I2C_ADDR_8BIT = 0x10 << 1;  // = 0x20
};

enum class TPBotLineState
{
    Black = 0,  // sensor reads 0: on a dark line
    White = 1   // sensor reads 1: off the line / on light surface
};

enum class TPBotLineSide
{
    Left,
    Right
};

// Combined state of both sensors, matching TrackingState in the pxt source.
enum class TPBotTrackingState
{
    L_R_line,          // both on line (both Black)
    L_unline_R_line,   // left off line, right on line
    L_line_R_unline,   // left on line, right off line
    L_R_unline         // both off line (both White)
};

class TPBotLineSensors
{
public:
    explicit TPBotLineSensors(MicroBit &ubit) : uBit(ubit)
    {
        uBit.io.P13.setPull(PullMode::None);
        uBit.io.P14.setPull(PullMode::None);
    }

    TPBotLineState readLeft()
    {
        return uBit.io.P13.getDigitalValue() ? TPBotLineState::White : TPBotLineState::Black;
    }

    TPBotLineState readRight()
    {
        return uBit.io.P14.getDigitalValue() ? TPBotLineState::White : TPBotLineState::Black;
    }

    // Mirrors trackSide(): true if the given side currently reads the
    // given state.
    bool trackSide(TPBotLineSide side, TPBotLineState state)
    {
        TPBotLineState actual = (side == TPBotLineSide::Left) ? readLeft() : readRight();
        return actual == state;
    }

    TPBotTrackingState getTrackingState()
    {
        bool leftOnLine = (readLeft() == TPBotLineState::Black);
        bool rightOnLine = (readRight() == TPBotLineState::Black);

        if (leftOnLine && rightOnLine)   return TPBotTrackingState::L_R_line;
        if (!leftOnLine && rightOnLine)  return TPBotTrackingState::L_unline_R_line;
        if (leftOnLine && !rightOnLine)  return TPBotTrackingState::L_line_R_unline;
        return TPBotTrackingState::L_R_unline;
    }

private:
    MicroBit &uBit;
};

#endif
