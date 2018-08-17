#!/bin/bash
TIME_GIT_URL=https://github.com/PaulStoffregen/Time.git

set -e

cd libraries

if ! test -e downloaded1 ; then
    git clone ${TIME_GIT_URL} Time
    touch downloaded1
fi

