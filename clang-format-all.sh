#!/bin/bash -e
git ls-files -- \*.cpp \*.h \*.hpp \*.mm | xargs clang-format -i
