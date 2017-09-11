
# The Tigrillo Robot Control Library
This library implements basic functions to read external sensors and actuate
motors over UART with a Tigrillo robot. It is Platform dependent and has no
direct use for another platform.

## Requirements
 - Boards: tested on a Raspberry Pi (ARM Cortex-A53) connected to an OpenCM
 board to control Dynamixel servomotors ARM. For more documentation on the
 platforms, check the folder platforms
 - OS: tested on Ubuntu Mate 16.04.2
 - Python dependencies on the RPI

## API

### *Sensors* Module

- **init ()**:
 Start the daemon that will listen to UART an i2c to read sensors values and
 queue them.

- **reset ()**:
 Set all sensors values to zero in the current robot configuration.

- **getLast (num=1)**:
 Return an array with the last *num* sensors values in the queue. Each sensor
 instance is a dictionary with a timestamp and all sensors names and values.


### *Actuators* Module

- **init ()**:
 Init actuation configuration.

- **reset (default=None)**:
 Reset all actuators to their default position. *default* is a dictionary with
 the actuator names and values. If not provided, the default value will be used.

- **updatePos (pos)**:
 Set all actuators to a given position. *pos* is a dictionary with all the actuators
 names and values.
