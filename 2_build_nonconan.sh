#!/bin/bash -e

rm -rf bd
cmake -S thirdparty -B bd -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=id
cmake --build bd --config Debug -j
cmake bd -DCMAKE_BUILD_TYPE=Release
cmake --build bd --config Release -j
