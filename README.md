This repo is a working model of a TPBot V1 being controlled by a 8BitDo SN30 Pro through a 8BitDo USB 2 Dongle.

The control is from a Jupyter Notebook on a Raspberry Pi.

It listens to the 8BitDo Joystick and passes on the message to a hard wired BBCMicrobit V2.2. Called base.

base then passes the message as bytes across the BBCMicrobit Radio and the robot responds.

A key learning point is that the TPBot V1 must be off when the new firmware is sent to it and then the main power is then switched on.

This ensures the CODAL I2C refreshes, otherwise I2C comms are not possible.

