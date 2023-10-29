#!/bin/bash -e

cmake -S build_nonconan -B bd -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=i
cmake --build bd --config Debug -j
cmake bd -DCMAKE_BUILD_TYPE=Release
cmake --build bd --config Release -j
