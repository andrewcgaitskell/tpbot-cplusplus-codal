import serial
import re

ser = serial.Serial('/dev/ttyACM1', 115200, timeout=1)  # adjust port as needed

# Matches TPBot.TrackingState in elecfreaks/pxt-TPBot (V1.ts / V2.ts):
#   L_R_line          = 0  -> both sensors on line (both Black)
#   L_unline_R_line    = 1  -> left off line, right on line
#   L_line_R_unline    = 2  -> left on line, right off line
#   L_R_unline         = 3  -> both sensors off line (both White)
TRACKING_STATES = {
    0: "L_R_line",
    1: "L_unline_R_line",
    2: "L_line_R_unline",
    3: "L_R_unline",
}

# base/main.cpp prints two kinds of telemetry line, plus '#'-prefixed
# debug/status lines we just pass through. These regexes match the
# uBit.serial.printf formats directly, so if the format string on the
# robot/base side changes, update the pattern here to match.
SENSOR_RE = re.compile(
    r"robot=(?P<robot>\d+) left=(?P<left>black|white) right=(?P<right>black|white) state=(?P<state>\d+)"
)
LOOP_TIME_RE = re.compile(
    r"robot=(?P<robot>\d+) loop_ms=(?P<loop_ms>-?\d+) max_loop_ms=(?P<max_loop_ms>-?\d+)"
)


def parse_line(line):
    """Parse one line of base station serial output into a dict, or None
    if it's a debug/'#' line or doesn't match a known format."""
    line = line.strip()
    if not line or line.startswith('#'):
        return None

    m = SENSOR_RE.match(line)
    if m:
        state_code = int(m.group('state'))
        return {
            "type": "sensor",
            "robot": int(m.group('robot')),
            "left": m.group('left'),
            "right": m.group('right'),
            "state": TRACKING_STATES.get(state_code, f"UNKNOWN({state_code})"),
        }

    m = LOOP_TIME_RE.match(line)
    if m:
        return {
            "type": "loop_time",
            "robot": int(m.group('robot')),
            "loop_ms": int(m.group('loop_ms')),
            "max_loop_ms": int(m.group('max_loop_ms')),
        }

    return None


def format_reading(reading):
    if reading["type"] == "sensor":
        return (f"robot {reading['robot']}: left={reading['left']} "
                f"right={reading['right']} state={reading['state']}")
    if reading["type"] == "loop_time":
        return (f"robot {reading['robot']}: loop={reading['loop_ms']}ms "
                f"max={reading['max_loop_ms']}ms")
    return str(reading)


if __name__ == "__main__":
    print(f"Listening on {ser.port} @ {ser.baudrate}...")
    while True:
        raw = ser.readline().decode('ascii', errors='ignore')
        if not raw:
            continue  # readline() timed out with nothing received

        if raw.strip().startswith('#'):
            print(raw.strip())  # pass base station debug/status lines through as-is
            continue

        reading = parse_line(raw)
        if reading is not None:
            print(format_reading(reading))
        elif raw.strip():
            print(f"# unrecognised line: {raw.strip()}")

