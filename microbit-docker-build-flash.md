# micro:bit v2: Docker build → OpenOCD flash workflow

How a C++ CODAL project (from
[lancaster-university/microbit-v2-samples](https://github.com/lancaster-university/microbit-v2-samples))
is built inside Docker and flashed onto a micro:bit v2, on Ubuntu.

## The working loop

```bash
docker run --rm -v "$(pwd)":/opt/microbit-samples -w /opt/microbit-samples microbit-builder python3 build.py
openocd -f interface/cmsis-dap.cfg -f target/nrf52.cfg -c "program MICROBIT.hex verify reset exit"
```

Run both from inside the `microbit-v2-samples/` folder. Edit `source/main.cpp`
(CODAL also picks up any other `.c`/`.cpp`/`.h` files placed in `source/`),
then re-run the two commands above.

---

## 1. Build the toolchain image (one-off)

The repo's Dockerfile is multi-stage — the stage with the actual compiler
toolchain (`gcc-arm-embedded`, `cmake`, `python3`, `git`, etc.) is named
`builder`. Tag that stage specifically:

```bash
docker build --target builder -t microbit-builder .
```

Only re-run this if the Dockerfile itself changes — not for ordinary source edits.

## 2. Build by bind-mounting live source (every edit)

```bash
docker run --rm \
  -v "$(pwd)":/opt/microbit-samples \
  -w /opt/microbit-samples \
  microbit-builder \
  python3 build.py
```

- `-v "$(pwd)":/opt/microbit-samples` — maps the current host folder onto the
  path the container expects. This is a live view, not a copy, so edits made
  on the host are picked up immediately with no image rebuild.
- `-w /opt/microbit-samples` — sets that as the working directory.
- `python3 build.py` — the same build script the Dockerfile calls, run
  against the live files.
- `--rm` — the container is discarded after the build finishes; only the
  host files (source + build output) persist.

Output files `MICROBIT.hex` and `MICROBIT.bin` land directly in the project
folder on the host.

## 3. One-time USB permission setup

```bash
sudo tee /etc/udev/rules.d/50-pyocd.rules > /dev/null << 'EOF'
# CMSIS-DAP / micro:bit DAPLink
ATTRS{idVendor}=="0d28", ATTRS{idProduct}=="0204", MODE="0666"
EOF

sudo udevadm control --reload-rules
sudo udevadm trigger
```

Unplug and replug the micro:bit afterwards so the rule takes effect.

## 4. Flash with OpenOCD

```bash
sudo apt install -y openocd
openocd -f interface/cmsis-dap.cfg -f target/nrf52.cfg -c "program MICROBIT.hex verify reset exit"
```

Successful output looks like:

```
** Programming Started **
...
** Programming Finished **
** Verify Started **
** Verified OK **
** Resetting Target **
```

## 5. Combined script

```bash
#!/bin/bash
set -e
docker run --rm \
  -v "$(pwd)":/opt/microbit-samples \
  -w /opt/microbit-samples \
  microbit-builder \
  python3 build.py
openocd -f interface/cmsis-dap.cfg -f target/nrf52.cfg -c "program MICROBIT.hex verify reset exit"
echo "Flashed."
```

## 6. VS Code `tasks.json`

```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "Build & Flash",
      "type": "shell",
      "command": "docker run --rm -v \"$(pwd)\":/opt/microbit-samples -w /opt/microbit-samples microbit-builder python3 build.py && openocd -f interface/cmsis-dap.cfg -f target/nrf52.cfg -c \"program MICROBIT.hex verify reset exit\"",
      "group": { "kind": "build", "isDefault": true }
    }
  ]
}
```

`Ctrl+Shift+B` runs the full build-and-flash cycle.

## Prerequisites

- **Docker** — user added to the `docker` group (`sudo usermod -aG docker $USER`) so commands run without `sudo`.
- **VS Code** — installed via Microsoft's official apt repo.
- **OpenOCD** — installed via `apt`.

## Open item

Breakpoint debugging via Cortex-Debug in VS Code isn't set up yet — needs a
`.elf` file with debug symbols. Check whether `python3 build.py` produces one
alongside `MICROBIT.hex`/`MICROBIT.bin` (`ls *.elf` after a build) before
wiring up `launch.json`.
