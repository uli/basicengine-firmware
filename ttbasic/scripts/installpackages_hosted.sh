#!/bin/bash
PACKAGES="g++ libsdl1.2-dev"

for p in ${PACKAGES} ; do
  if ! dpkg -s "$p" >/dev/null 2>&1 ; then
	sudo apt-get -y install "$p"
  fi
done
