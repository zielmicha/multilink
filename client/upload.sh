#!/bin/bash
set -e
if [ "$1" = "" ]; then
    echo "provide target hostname"
    exit 1
fi
ninja -C ../build
rsync --progress -v ../build/app *.py "$1":mbuild/
