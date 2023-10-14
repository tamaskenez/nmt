#!/bin/bash -e

cmake -H. -Bb -DCMAKE_PREFIX_PATH=$PWD/i -DCMAKE_INSTALL_PREFIX=o -DCMAKE_BUILD_TYPE=Debug "$@"
cmake --build b --target install --config Debug -j
