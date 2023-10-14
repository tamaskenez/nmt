#!/bin/bash -e

conan install conanfile.txt -b missing -pr:b default -if i/cmake -of i/cmake -s build_type=Debug
# conan install conanfile.txt -b missing -pr:b default -if i/cmake -of i/cmake -s build_type=Release
