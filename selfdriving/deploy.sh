#!/bin/bash
# Lives in the selfdriving folder. Copies the selfdriving
# robot source files into the microbit-selfdriving project source directory,
# overwriting existing files.
#
# Usage: source deploy.sh
#
# Everything printed to the screen is also written to a log file next to
# this script, and the window stays open at the end (success or failure)
# until you press enter - so nothing gets lost if the terminal auto-closes.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOGFILE="$SCRIPT_DIR/deploy_selfdriving_$(date +%Y%m%d_%H%M%S).log"

# Mirror everything (stdout + stderr) to the log file as well as the screen.
exec > >(tee -a "$LOGFILE") 2>&1

STATUS=0
trap 'STATUS=$?; echo; echo "=== Script exited with status $STATUS ==="; echo "Log saved to: $LOGFILE"; echo ">>> Press enter to close this window"; read -r _ < /dev/tty || true' EXIT

pause() {
    echo ">>> $1 [press enter to continue]"
    read -r _ < /dev/tty || true
}

echo "--- Resolving paths ---"
SELFDRIVING_SRC="$SCRIPT_DIR"
SELFDRIVING_DEST="/home/picontrol/BBCMicrobit/microbit-selfdriving/source"

echo "SCRIPT_DIR         = $SCRIPT_DIR"
echo "SELFDRIVING_SRC    = $SELFDRIVING_SRC"
echo "SELFDRIVING_DEST   = $SELFDRIVING_DEST"
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

echo "--- Contents of SELFDRIVING_SRC before copy (will copy .cpp and .h files) ---"
ls -la "$SELFDRIVING_SRC"/*.{cpp,h} 2>/dev/null || echo "(no .cpp or .h files found)"
pause "Review selfdriving source files above"

echo "--- Contents of SELFDRIVING_DEST before copy (will be overwritten) ---"
ls -la "$SELFDRIVING_DEST"
pause "Review selfdriving dest files above - about to overwrite .cpp and .h files"

echo "Copying selfdriving .cpp and .h files: $SELFDRIVING_SRC -> $SELFDRIVING_DEST"
if ! cp -fv "$SELFDRIVING_SRC"/*.{cpp,h} "$SELFDRIVING_DEST"/ 2>/dev/null; then
    echo "Error: copy of selfdriving files failed"
    exit 1
fi
echo "--- Contents of SELFDRIVING_DEST after copy ---"
ls -la "$SELFDRIVING_DEST"

pause "Done."
