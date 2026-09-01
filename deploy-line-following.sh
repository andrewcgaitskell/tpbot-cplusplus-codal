#!/bin/bash
# Lives in the root of tpbot-cplusplus-codal. Copies the linefollowing
# robot/base source files into the microbit-robot and microbit-base
# project source directories, overwriting existing files.
#
# Usage: ./deploy_linefollowing.sh
set -euo pipefail

pause() {
    read -rp ">>> $1 [press enter to continue] "
}

echo "--- Resolving paths ---"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROBOT_SRC="$SCRIPT_DIR/linefollowing/robot"
BASE_SRC="$SCRIPT_DIR/linefollowing/base"
ROBOT_DEST="/home/picontrol/BBCMicrobit/microbit-robot/source"
BASE_DEST="/home/picontrol/BBCMicrobit/microbit-base/source"

echo "SCRIPT_DIR = $SCRIPT_DIR"
echo "ROBOT_SRC  = $ROBOT_SRC"
echo "BASE_SRC   = $BASE_SRC"
echo "ROBOT_DEST = $ROBOT_DEST"
echo "BASE_DEST  = $BASE_DEST"
pause "Paths resolved above - do they look right?"

echo "--- Checking directories exist ---"
for dir in "$ROBOT_SRC" "$BASE_SRC" "$ROBOT_DEST" "$BASE_DEST"; do
    if [ ! -d "$dir" ]; then
        echo "Error: directory not found: $dir" >&2
        exit 1
    fi
    echo "OK: $dir"
done
pause "All four directories exist - continue to copy?"

echo "--- Contents of ROBOT_SRC before copy ---"
ls -la "$ROBOT_SRC"
pause "Review robot source files above"

echo "--- Contents of ROBOT_DEST before copy (will be overwritten) ---"
ls -la "$ROBOT_DEST"
pause "Review robot dest files above - about to overwrite"

echo "Copying robot files: $ROBOT_SRC -> $ROBOT_DEST"
cp -fv "$ROBOT_SRC"/* "$ROBOT_DEST"/
echo "--- Contents of ROBOT_DEST after copy ---"
ls -la "$ROBOT_DEST"
pause "Robot copy done - continue to base?"

echo "--- Contents of BASE_SRC before copy ---"
ls -la "$BASE_SRC"
pause "Review base source files above"

echo "--- Contents of BASE_DEST before copy (will be overwritten) ---"
ls -la "$BASE_DEST"
pause "Review base dest files above - about to overwrite"

echo "Copying base files: $BASE_SRC -> $BASE_DEST"
cp -fv "$BASE_SRC"/* "$BASE_DEST"/
echo "--- Contents of BASE_DEST after copy ---"
ls -la "$BASE_DEST"

echo "Done."

