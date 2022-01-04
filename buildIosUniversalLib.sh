#!/bin/bash

# Set -DPLATFORM to
#    OS = Build for iPhoneOS.
#    OS64 = Build for arm64 iphoneOS.
#    OS64COMBINED = Build for arm64 x86_64 iphoneOS. Combined into FAT STATIC lib (supported on 3.14+ of CMakewith "-G Xcode" argument ONLY)
#    SIMULATOR = Build for x86 i386 iphoneOS Simulator.
#    SIMULATOR64 = Build for x86_64 iphoneOS Simulator.
#    SIMULATORARM64 = Build for arm64 iphoneOS Simulator.
#    TVOS = Build for arm64 tvOS.
#    TVOSCOMBINED = Build for arm64 x86_64 tvOS. Combined into FAT STATIC lib (supported on 3.14+ of CMake with "-G Xcode" argument ONLY)
#    SIMULATOR_TVOS = Build for x86_64 tvOS Simulator.
#    WATCHOS = Build for armv7k arm64_32 for watchOS.
#    WATCHOSCOMBINED = Build for armv7k arm64_32 x86_64 watchOS. Combined into FAT STATIC lib (supported on 3.14+ of CMake with "-G Xcode" argument ONLY)
#    SIMULATOR_WATCHOS = Build for x86_64 for watchOS Simulator.
#    MAC = Build for x86_64 macOS.
#    MAC_ARM64 = Build for Apple Silicon macOS.
#    MAC_CATALYST = Build for x86_64 macOS with Catalyst support (iOS toolchain on macOS).
#                   Note: The build argument "MACOSX_DEPLOYMENT_TARGET" can be used to control min-version of macOS
#    MAC_CATALYST_ARM64 = Build for Apple Silicon macOS with Catalyst support (iOS toolchain on macOS).
#                         Note: The build argument "MACOSX_DEPLOYMENT_TARGET" can be used to control min-version of macOS

PLATFORMS=("OS" "SIMULATOR64")
COMBINED_PLATFORMS=("OS64COMBINED" "TVOSCOMBINED" "WATCHOSCOMBINED") 

BUILD_DIR="${PWD}/build_ios"
FAT_LIB_DIR="ALL"
OPENSMILE_LIB_NAME="libopensmile.a"
SMILEAPI_LIB_NAME="libSMILEapi.a"
OPENSMILE_LIB_PATHS=""
SMILEAPI_LIB_PATHS=""

mkdir $BUILD_DIR
rm -rf $BUILD_DIR/*
cd $BUILD_DIR
mkdir $FAT_LIB_DIR

source ../build_flags.sh

# NOTICE:
# Because Xcode can not build some third party library (e.g., XGBoost) due to
# the empty container library issue, the '-G Xcode' option is used only for the
# *COMBINED platforms.

echo "Building for platforms: ${PLATFORMS[@]}"
for PLATFORM in "${PLATFORMS[@]}"; do
  mkdir $PLATFORM
  cd $PLATFORM

  INSTALL_PATH="${BUILD_DIR}/${PLATFORM}/install"
  ios_flags=(
    -DPLATFORM=$PLATFORM         # used in ios.toolchain.cmake
    -DIS_IOS_PLATFORM=ON
    -DENABLE_BITCODE=ON
    -DSMILEAPI_STATIC_LINK=ON
    -DCMAKE_TOOLCHAIN_FILE=./cmake/ios.toolchain.cmake
    -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH
  )

  if [[ " ${COMBINED_PLATFORMS[@]} " =~ " ${PLATFORM} " ]]; then
    ios_flags+=(
        -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=OFF
    )
    cmake "${cmake_flags[@]}" "${ios_flags[@]}" -DBUILD_FLAGS="$build_flags" "$@" -G Xcode ../..
    cmake --build . --target install --config Release
  else
    cmake "${cmake_flags[@]}" "${ios_flags[@]}" -DBUILD_FLAGS="$build_flags" "$@" ../..
    cmake --build . --target install
  fi

  OPENSMILE_LIB_PATHS+="${INSTALL_PATH}/lib/${OPENSMILE_LIB_NAME} "
  SMILEAPI_LIB_PATHS+="${INSTALL_PATH}/lib/${SMILEAPI_LIB_NAME} "

  # Copy header files resulting from build to FAT_LIB_DIR (git_version.hpp)
  cp -rv "${BUILD_DIR}/${PLATFORM}/src/include" "${BUILD_DIR}/${FAT_LIB_DIR}"

  cd ..
done

# Create universal libraries
cd "${BUILD_DIR}/${FAT_LIB_DIR}"
xcrun lipo -create $OPENSMILE_LIB_PATHS -output "${OPENSMILE_LIB_NAME}"
xcrun lipo -create $SMILEAPI_LIB_PATHS -output "${SMILEAPI_LIB_NAME}"
