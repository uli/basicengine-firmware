# ![BASIC Engine logo](./doc/basic_engine_small.png) BASIC Engine Firmware (ALPHA!)

[This branch is under development!]

This branch implements a version of Engine BASIC for Allwinner H3 SoCs that
runs in the Jailhouse bare metal hypervisor. It runs alongside a Linux
kernel that provides it with input, file system and networking services.

This dramatically increases hardware compatibility and file system
performance while retaining the open and real-time nature of bare metal
Engine BASIC.

The [BASIC Engine](https://basicengine.org/) is a BASIC programming
environment based on low-cost single-board computers that features advanced
2D color graphics and sound capabilities, roughly comparable to early to
mid-1990s computers and video game consoles.  It can be easily installed on
an SD card and immediately used on "Orange Pi" Allwinner H3 boards typically
costing less than 10 Euros.

More information on can be found on the [BASIC Engine web site](https://basicengine.org)
and the [BASIC Engine web forum](https://betest.freeflarum.com/).

Demo programs can be found at the [BASIC Engine demos](https://github.com/uli/basicengine-demos)
repository.

## Screenshots

[These screenshots are from the original, custom-built BASIC Engine hardware.]

![Shmup](./doc/screenshots/screen_shmup.png)
![Zork](./doc/screenshots/screen_zork.png)
![Boot screen](./doc/screenshots/screen_boot.png)

## Videos

[These videos are from the original, custom-built BASIC Engine hardware.]

Click on the thumbnails below to watch some demo videos:
[![Shmup](http://img.youtube.com/vi/WEeHVyWH8rQ/0.jpg)](http://www.youtube.com/watch?v=WEeHVyWH8rQ "BASIC Engine Shmup Demo")
[![Tetris](http://img.youtube.com/vi/0ZsucdE6l2o/0.jpg)](http://www.youtube.com/watch?v=0ZsucdE6l2o "BASIC Engine Tetris Demo")

## Compiling

Run `make` for instructions on how to compile the firmware for various targets.

Building on both SDL 1.2 and H3 requires GNU make and Python 3 with the polib library.

Building on the H3 platform requires the [allwinner-bare-metal framework](https://github.com/uli/allwinner-bare-metal)
(opi branch), which in turn requires a toolchain to be built with
crosstool-ng. More information can be found in the allwinner-bare-metal
repository.

Building for SDL 1.2 requires the usual dependencies. This version has been
tested exclusively on Linux/AMD64, but is not unlikely to work (possibly with some
amendments) on other platforms.
