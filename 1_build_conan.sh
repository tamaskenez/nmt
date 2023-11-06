#!/bin/bash -e

conan install conanfile.txt -b missing -pr:b default -if id/cmake -of id/cmake -s build_type=Debug -s os.version=13.0
# conan install conanfile.txt -b missing -pr:b default -if id/cmake -of id/cmake -s build_type=Release


readonly qt6config=id/cmake/Qt6Config.cmake
grep -q "__qt_internal_cmake_apple_support_files_path" <$qt6config || \
	echo 'set(__qt_internal_cmake_apple_support_files_path "${qt_PACKAGE_FOLDER_DEBUG}/lib/cmake/Qt6/macos")' >> $qt6config
