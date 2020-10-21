#!/bin/bash

# Set -DPLATFORM to "SIMULATOR" to build for iOS simulator 32 bit (i386) DEPRECATED
# Set -DPLATFORM to "SIMULATOR64" (example above) to build for iOS simulator 64 bit (x86_64)
# Set -DPLATFORM to "OS" to build for Device (armv7, armv7s, arm64)
# Set -DPLATFORM to "OS64" to build for Device (arm64)
# Set -DPLATFORM to "OS64COMBINED" to build for Device & Simulator (FAT lib) (arm64, x86_64)
# Set -DPLATFORM to "TVOS" to build for tvOS (arm64)
# Set -DPLATFORM to "TVOSCOMBINED" to build for tvOS & Simulator (arm64, x86_64)
# Set -DPLATFORM to "SIMULATOR_TVOS" to build for tvOS Simulator (x86_64)
# Set -DPLATFORM to "WATCHOS" to build for watchOS (armv7k, arm64_32)
# Set -DPLATFORM to "WATCHOSCOMBINED" to build for watchOS & Simulator (armv7k, arm64_32, i386)
# Set -DPLATFORM to "SIMULATOR_WATCHOS" to build for watchOS Simulator (i386)

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
        -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED=OFF
        -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY=""
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
