#!/bin/bash
# Lives in the selfdriving folder. Copies the selfdriving
# robot source files into the microbit-robot project source directory,
# overwriting existing files.
#
# Usage: source deploy.sh
#
# Everything printed to the screen is also written to a log file next to
# this script, and the window stays open at the end (success or failure)
# until you press enter - so nothing gets lost if the terminal auto-closes.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOGFILE="$SCRIPT_DIR/deploy_robot_$(date +%Y%m%d_%H%M%S).log"

# Mirror everything (stdout + stderr) to the log file as well as the screen.
exec > >(tee -a "$LOGFILE") 2>&1

STATUS=0
trap 'STATUS=$?; echo; echo "=== Script exited with status $STATUS ==="; echo "Log saved to: $LOGFILE"; echo ">>> Press enter to close this window"; read -r _ < /dev/tty || true' EXIT

pause() {
    echo ">>> $1 [press enter to continue]"
    read -r _ < /dev/tty || true
}

echo "--- Resolving paths ---"
echo "my script source /home/picontrol/BBCMicrobit/tpbot-cplusplus-codal/robot"

ROBOT_SRC="$SCRIPT_DIR"
ROBOT_DEST="/home/picontrol/BBCMicrobit/microbit-robot/source"

echo "SCRIPT_DIR         = $SCRIPT_DIR"
echo "ROBOT_SRC    = $ROBOT_SRC"
echo "ROBOT_DEST   = $ROBOT_DEST"
pause "Paths resolved above - do they look right?"

echo "--- Checking directories exist ---"
for dir in "$SELFDRIVING_SRC" "$SELFDRIVING_DEST"; do
    echo "Checking: $dir"
    if [ ! -d "$dir" ]; then
        echo "Error: directory not found: $dir"
        echo "Parent directory contents:"
        ls -la "$(dirname "$dir")" || echo "(parent directory also missing)"
        exit 1
    fi
    echo "OK: $dir"
done
pause "All directories exist - continue to copy?"

echo "--- Contents of ROBOT_SRC before copy (will copy .cpp and .h files) ---"
ls -la "$ROBOT_SRC"/*.{cpp,h} 2>/dev/null || echo "(no .cpp or .h files found)"
pause "Review selfdriving source files above"

echo "--- Contents of SELFDRIVING_DEST before copy (will be overwritten) ---"
ls -la "$ROBOT_DEST"
pause "Review selfdriving dest files above - about to overwrite .cpp and .h files"

echo "Copying selfdriving .cpp and .h files: $ROBOT_SRC -> $ROBOT_DEST"
if ! cp -fv "$ROBOT_SRC"/*.{cpp,h} "$ROBOT_DEST"/ 2>/dev/null; then
    echo "Error: copy of selfdriving files failed"
    exit 1
fi
echo "--- Contents of SELFDRIVING_DEST after copy ---"
ls -la "$ROBOT_DEST"

pause "Done."
