#!/bin/bash

# prepare build directory
mkdir -p build
rm -rf build/*
cd build

# make sure -DSTATIC_LINK=OFF is set in build_flags.sh to enable plug-in support in openSMILE
source ../../build_flags.sh 

# use Ninja as build system if available, otherwise fall back to make
if [ -x "$(command -v ninja)" ]; then
    generator="Ninja"
else
    generator="Unix Makefiles"
fi

cmake -G "$generator" "${cmake_flags[@]}" -DBUILD_FLAGS="$build_flags" "$@" ..
cmake --build . -- -j 8

# copy plugin to SMILExtract's plugins directory
mkdir opensmile/progsrc/smilextract/plugins
cp libexampleplugin.so opensmile/progsrc/smilextract/plugins