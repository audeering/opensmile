#!/bin/bash

# prepare build directory
mkdir -p build
rm -rf build/*
cd build

source ../build_flags.sh 

# use Ninja as build system if available, otherwise fall back to make
if [ -x "$(command -v ninja)" ]; then
    generator="Ninja"
else
    generator="Unix Makefiles"
fi

cmake -G "$generator" "${cmake_flags[@]}" -DBUILD_FLAGS="$build_flags" "$@" ..
cmake --build . -- -j 8
