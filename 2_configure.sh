#!/bin/bash -e

cmake -H. -Bb -DCMAKE_PREFIX_PATH=$PWD/i "$@"

