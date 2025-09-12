#!/bin/bash
# Run a CMake build.

set -e

# clean build
rm -rf build

# configure for release
mkdir -p build
cd build
cmake .. -D CMAKE_BUILD_TYPE=Release  -D CMAKE_VERBOSE_MAKEFILE=ON -G Ninja 
cd ..

time cmake --build ./build --target all  --config Release -- -j 3

