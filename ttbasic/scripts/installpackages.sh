#!/bin/bash
PACKAGES="asciidoctor libc6-dev libncurses5-dev gperf python-serial exuberant-ctags"

for p in ${PACKAGES} ; do
  if ! dpkg -s "$p" >/dev/null 2>&1 ; then
	sudo apt-get -y install "$p"
  fi
done
