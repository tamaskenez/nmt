#!/bin/bash -e

if [[ $(uname -s)=~Darwin ]]; then
	readonly generator_option=-GXcode
fi
cmake -H. -Bb -DCMAKE_PREFIX_PATH=$PWD/id -DCMAKE_INSTALL_PREFIX=i -DCMAKE_BUILD_TYPE=Debug $generator_option \
	-D BUILD_FULL=0 -D BUILD_TESTING=1

