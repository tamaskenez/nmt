#!/bin/bash -e

cmake -H. -Bb2 -DCMAKE_PREFIX_PATH="$PWD/i;$PWD/j" -DCMAKE_INSTALL_PREFIX=j2 -DCMAKE_BUILD_TYPE=Debug -DUSE_INSTALLED_TOOL=1 "$@"
