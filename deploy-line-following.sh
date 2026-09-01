#!/bin/bash
# Lives in the root of tpbot-cplusplus-codal. Copies the linefollowing
# robot/base source files into the microbit-robot and microbit-base
# project source directories, overwriting existing files.
#
# Usage: ./deploy_linefollowing.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

ROBOT_SRC="$SCRIPT_DIR/linefollowing/robot"
BASE_SRC="$SCRIPT_DIR/linefollowing/base"

ROBOT_DEST="/home/picontrol/BBCMicrobit/microbit-robot/source"
BASE_DEST="/home/picontrol/BBCMicrobit/microbit-base/source"

for dir in "$ROBOT_SRC" "$BASE_SRC" "$ROBOT_DEST" "$BASE_DEST"; do
    if [ ! -d "$dir" ]; then
        echo "Error: directory not found: $dir" >&2
        exit 1
    fi
done

echo "Copying robot files: $ROBOT_SRC -> $ROBOT_DEST"
cp -f "$ROBOT_SRC"/* "$ROBOT_DEST"/

echo "Copying base files: $BASE_SRC -> $BASE_DEST"
cp -f "$BASE_SRC"/* "$BASE_DEST"/

echo "Done."
