#!/bin/bash -e

if [[ $(uname -s)=~Darwin ]]; then
	readonly generator_option=-GXcode
fi
cmake -H. -Bb1 -D "CMAKE_PREFIX_PATH=$PWD/id;$PWD/i0"
	-DCMAKE_INSTALL_PREFIX=i1 -DCMAKE_BUILD_TYPE=Debug $generator_option
