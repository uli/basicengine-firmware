#!/bin/bash
PACKAGES=asciidoctor

for p in ${PACKAGES} ; do
  if ! dpkg -s "$p" >/dev/null 2>&1 ; then
	sudo apt-get install "$p"
  fi
done
