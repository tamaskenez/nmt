#!/bin/bash -e

# Build and install nmt-only in config time and use that to config the full project.

if [[ $(uname -s)=~Darwin ]]; then
	readonly generator_option=-GXcode
fi
cmake -H. -Bb -DCMAKE_PREFIX_PATH=$PWD/id -DCMAKE_INSTALL_PREFIX=i -DCMAKE_BUILD_TYPE=Debug $generator_option \
	-D BUILD_FULL=1 -D BUILD_TESTING=1 --fresh

