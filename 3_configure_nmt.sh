#!/bin/bash -e

if [[ $(uname -s)=~Darwin ]]; then
	readonly generator_option=-GXcode
fi
cmake -H. -Bb0 -DCMAKE_PREFIX_PATH=$PWD/id -DCMAKE_INSTALL_PREFIX=i0 -DCMAKE_BUILD_TYPE=Debug $generator_option \
	-D BUILD_NMT=1 -D BUILD_ALL=0 -D BUILD_TESTING=1 --fresh

