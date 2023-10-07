#!/bin/bash -e

if [[ $(uname -s)=~Darwin ]]; then
	readonly extra_profile2="-pr ./build/conan_profiles/darwin"
else
	echo "Not darwin"
fi

conan install conanfile.txt -b missing -pr default $extra_profile -if i/cmake -of i/cmake -s build_type=Debug
conan install conanfile.txt -b missing -pr default $extra_profile -if i/cmake -of i/cmake -s build_type=Release
