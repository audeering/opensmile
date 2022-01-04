#!/bin/bash

# This file is part of openSMILE.
#
# Copyright (c) audEERING. All rights reserved.
# See the file COPYING for details on license terms.

build_flags=(
    # libsvm and dependent components (cLibsvmLiveSink)
    -DBUILD_LIBSVM

    # LSTM RNN components
    -DBUILD_RNN

    # cSvmSink
    -DBUILD_SVMSMO
)

cmake_flags=(
    # switches between release and debug builds
    -DCMAKE_BUILD_TYPE=Release

    # use Clang compiler. Comment out to use system-default C and C++ compilers.
    # Leave commented out when building for Android, as the NDK version of Clang is required.
    #-DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++

    # controls whether to build and link to libopensmile as static or shared library
    # static builds are preferred unless you need openSMILE plugins
    -DSTATIC_LINK=ON

    # tune compiler optimizations to the processor of this machine.
    # This ensures that openSMILE runs with optimal performance on the machine it was
    # compiled on but it may not run at all on other machines.
    # Disable if the compiled binary needs to be portable.
    -DMARCH_NATIVE=OFF

    # whether to compile with PortAudio support
    -DWITH_PORTAUDIO=OFF

    # whether to compile with FFmpeg support
    # 1. download ffmpeg source distribution
    # 2. run: mkdir build ; cd build
    #         ../configure --enable-shared --disable-static
    #         make ; sudo make install
    -DWITH_FFMPEG=OFF

    # whether to compile with OpenCV support
    -DWITH_OPENCV=OFF

    # whether to compile with OpenSL ES support (only applies when building for Android)
    -DWITH_OPENSLES=ON
)

build_flags="${build_flags[@]}"
