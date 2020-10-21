#!/bin/bash

# prepare build directory
mkdir -p build_android
rm -rf build_android/*
cd build_android

source ../build_flags.sh 

# you may want to override the Android ABI and API level via the script arguments
android_flags=(
    -DCMAKE_TOOLCHAIN_FILE=${ANDROID_HOME}/ndk-bundle/build/cmake/android.toolchain.cmake
    -DANDROID_ABI=x86_64
    -DANDROID_NATIVE_API_LEVEL=28
)

cmake -G "Ninja" "${cmake_flags[@]}" "${android_flags[@]}" -DBUILD_FLAGS="$build_flags" "$@" ..
cmake --build . -- -j 8
