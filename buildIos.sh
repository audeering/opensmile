#!/bin/bash

# prepare build directory
mkdir -p build_ios
rm -rf build_ios/*
cd build_ios

source ../build_flags.sh 

# you may want to override PLATFORM via the script arguments
# see cmake/ios.toolchain.cmake for the supported platform identifiers
ios_flags=(
    -DPLATFORM=OS
    -DIS_IOS_PLATFORM=ON
    -DSMILEAPI_STATIC_LINK=ON # required since shared libraries are not supported on iOS
    -DCMAKE_TOOLCHAIN_FILE=./cmake/ios.toolchain.cmake
    -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=OFF
)

cmake -G "Xcode" "${cmake_flags[@]}" "${ios_flags[@]}" -DBUILD_FLAGS="$build_flags" "$@" ..
cmake --build . --config Release
