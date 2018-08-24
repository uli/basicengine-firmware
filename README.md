# ![BASIC Engine logo](./doc/basic_engine_small.png) BASIC Engine Firmware (ALPHA!)

The [BASIC Engine](https://basicengine.org/) is a very low-cost single-board home computer with advanced
2D color graphics and sound capabilities, roughly comparable to late-1980s
or early-1990s computers and video game consoles.  It can be built at home
without special skills or tools and using readily available components for
under 10 Euros in parts, or mass-produced for even less.

More information on can be found on the [BASIC Engine web site](https://basicengine.org).

The hardware design is maintained at the [BASIC Engine PCB](https://github.com/uli/basicengine-pcb)
repository, and demo programs can be found at the [BASIC Engine demos](https://github.com/uli/basicengine-demos)
repository.

## Screenshots

![Shmup](./doc/screenshots/screen_shmup.png)
![Zork](./doc/screenshots/screen_zork.png)
![Boot screen](./doc/screenshots/screen_boot.png)

## Videos

Click on the thumbnails below to watch some demo videos:
[![Shmup](http://img.youtube.com/vi/WEeHVyWH8rQ/0.jpg)](http://www.youtube.com/watch?v=WEeHVyWH8rQ "BASIC Engine Shmup Demo")
[![Tetris](http://img.youtube.com/vi/0ZsucdE6l2o/0.jpg)](http://www.youtube.com/watch?v=0ZsucdE6l2o "BASIC Engine Tetris Demo")

Find out more at the [BASIC Engine web site](https://basicengine.org/).

## Compiling

Run `make` for instructions on how to compile the firmware for various targets.

The build system will currently only work on x64 Debian-like systems. If you want to
change them to work with your system, you will have to adapt these shell scripts in
`ttbasic/scripts`:

- `installpackages.sh`
- `installpackages_hosted.sh`
- `getesp8266.sh`
- `getesp32.sh`
