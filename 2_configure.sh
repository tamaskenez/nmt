#!/bin/bash -e

cmake -H. -Bb -DCMAKE_PREFIX_PATH=$PWD/i -DCMAKE_INSTALL_PREFIX=j -DCMAKE_BUILD_TYPE=Debug "$@"

