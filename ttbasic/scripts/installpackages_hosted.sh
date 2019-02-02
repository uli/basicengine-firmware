#!/bin/bash
PACKAGES="g++ libsdl1.2-dev"

if [ `lsb_release -is` = "Fedora" ]; then
    PACKAGES="libstdc++-devel SDL-devel"
    sudo dnf install -y $PACKAGES
else
    for p in ${PACKAGES} ; do
      if ! dpkg -s "$p" >/dev/null 2>&1 ; then
    	sudo apt-get -y install "$p"
      fi
    done

fi
