# Gannet Thruster Firmware

## Description
The thruster firmware ROS package created for the Gannet autonomous boat.

There are two modes:
- RF Receiver Mode
- Serial Mode

The RF mode will be prioritized and will override messages received over a
serial connection. It uses an input signal on the Engine/Gear channel of a
standard RC radio to know that the radio is connected.

## Usage
The package uses Arduino CMake bundled with Rosserial to be easily integrated
with the Catkin pipeline.

_*Note that a special Catkin flag must be set to upload it through the command
line_
