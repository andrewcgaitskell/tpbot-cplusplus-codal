#ifndef TPBOT_PROTOCOL_H
#define TPBOT_PROTOCOL_H

#include <stdint.h>
#include <string.h>

// Fixed-size binary packet sent over the radio link (base <-> robot).
// 6 bytes total, comfortably inside the 32-byte MICROBIT_RADIO_MAX_PACKET_SIZE
// limit, with headroom to add more commands/fields later without
// approaching that ceiling.
//
// This struct is only used on the radio hop. The Python <-> base serial
// link stays plain text (CSV), because that link needs to safely share
// space with human-readable '#'-prefixed debug output - packing raw
// binary bytes into a newline-delimited text stream is a good way to get
// a binary byte that happens to equal '\n' or '#' and silently corrupt
// framing.

struct __attribute__((packed)) RobotCommand
{
    uint8_t robotId;    // which robot this command targets (0 = broadcast to all)
    uint8_t command;    // see command IDs below
    int16_t value1;     // meaning depends on command, e.g. left wheel speed / left sensor
    int16_t value2;     // meaning depends on command, e.g. right wheel speed / right sensor
};

static const int ROBOT_COMMAND_SIZE = sizeof(RobotCommand);  // 6

// Command IDs. Add new ones here as the protocol grows - there's plenty
// of headroom left in the 32-byte radio packet limit for more fields or
// commands before this needs revisiting.
enum
{
    CMD_SET_WHEELS     = 1,  // base -> robot: value1/value2 = left/right wheel speed (unused, no joystick)
    CMD_POLL_SENSORS   = 2,  // base -> robot: request a line sensor reading (value1/value2 unused)
    CMD_SENSOR_REPORT  = 3,  // robot -> base: value1/value2 = left/right (1 = black, 0 = white)
};

// Packs a command into buf (caller must provide at least ROBOT_COMMAND_SIZE
// bytes). Returns the number of bytes written.
inline int packRobotCommand(uint8_t *buf, uint8_t robotId, uint8_t command,
                             int16_t value1, int16_t value2)
{
    RobotCommand pkt = { robotId, command, value1, value2 };
    memcpy(buf, &pkt, sizeof(pkt));
    return sizeof(pkt);
}

// Unpacks a command from buf/len into out. Returns false if len is too
// short to contain a full packet.
inline bool unpackRobotCommand(const uint8_t *buf, int len, RobotCommand &out)
{
    if (len < (int)sizeof(RobotCommand))
        return false;
    memcpy(&out, buf, sizeof(RobotCommand));
    return true;
}

#endif
