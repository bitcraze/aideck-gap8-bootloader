# GAP8 second stage bootloader and updater

**NOTE**: This is still under heavy development and lots of things might change
before it's considered stable.

## About

This bootloader has two main functions:

* Updating the user application on the GAP8
* Booting the user application on the GAP8

The bootloader is considered safe in the sense that it will always start up
and then either update or update and start the user application. Even if an update
fails and the user application is corrupt, the bootloader will start up.

The bootloader gives full access to the flash (except for the part where the bootloader
is located), which means it's possible to update both application and partition tables.
Basically you're "remote" flashing the same image you would flash via JTAG after building.

## Design details

### Memory and flash structure

TODO

### Firmware binary structure

The firmware image produced from the GAP8 toolchain (the one ending in .img) contains
information about what segments should be loaded into RAM and where to start executing it.

### Communication

TODO

## Utilities

### bootload.py

This script can be used to bootload the GAP8 via WiFi. Note, for this to work you need the AI-deck
to already be connected to the WiFi.

```bash
$ python3 bootload.py -h
usage: bootload.py [-h] [-n ip] [-p port] image

Connect to AI-deck JPEG streamer example

positional arguments:
  image       firmware image to flash

optional arguments:
  -h, --help  show this help message and exit
  -n ip       AI-deck IP
  -p port     AI-deck port
```

### check-app-image.py

Because of the risk of overwriting the running bootloader in RAM when loading the user
application, this utility can be used to analyze the user firmware image. It checks the
memory areas in the firmware image that will be loaded to RAM before jumping to the application
to see if these will overlap with the bootloader. If it overlaps the script will return a non-zero
result.

```bash
$ python3 check-app-image.py -h
usage: check-app-image.py [-h] image

Show GAP8 firmware image header information

positional arguments:
  image       firmware image to analyze

optional arguments:
  -h, --help  show this help message and exit
```
