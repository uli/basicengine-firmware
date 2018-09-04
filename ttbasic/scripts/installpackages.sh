#!/bin/bash

# install 32-bit runtime on ARM64 systems
if test `uname -m` == aarch64 && ! test -e /lib/arm-linux-gnueabihf/libz.so.1
then
  sudo dpkg --add-architecture armhf
  sudo apt-get update
  sudo apt-get -y install libc6:armhf libstdc++6:armhf zlib1g:armhf
fi

PACKAGES="asciidoctor libc6-dev libncurses5-dev gperf python-serial exuberant-ctags"

for p in ${PACKAGES} ; do
  if ! dpkg -s "$p" >/dev/null 2>&1 ; then
	sudo apt-get -y install "$p"
  fi
done
