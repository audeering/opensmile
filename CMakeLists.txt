cmake_minimum_required(VERSION 3.5.1)
if(NOT ${CMAKE_VERSION} VERSION_LESS "3.15")
    cmake_policy(SET CMP0091 NEW) # required for CMAKE_MSVC_RUNTIME_LIBRARY but it's only supported by CMake 3.15 or later
endif()
project(openSMILE)

option(STATIC_LINK "Build libopensmile as a static lib." ON)
option(MARCH_NATIVE "Tune compiler optimizations to the processor of this machine. Disable if the compiled binary needs to be portable." OFF)
option(WITH_PORTAUDIO "Compile with PortAudio support." OFF)
option(WITH_FFMPEG "Compile with FFmpeg support." OFF)
option(WITH_OPENSLES "Compile with OpenSL ES support (Android only)." OFF)
option(WITH_OPENCV "Compile with OpenCV support." OFF)
set(BUILD_FLAGS "" CACHE STRING "Build flags controlling which components of openSMILE are included in the build.")
set(PROFILE_GENERATE "" CACHE PATH "Path where to save profile information for profile-guided optimization.")
set(PROFILE_USE "" CACHE PATH "Path to profile information to use for profile-guided optimization.")

set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)

if(PROFILE_GENERATE AND PROFILE_USE)
    message(FATAL_ERROR "PROFILE_GENERATE and PROFILE_USE must not be set at the same time.")
endif()

set(WATCH_PLATFORMS WATCHOS WATCHOSCOMBINED SIMULATOR_WATCHOS)
if(IS_IOS_PLATFORM AND (PLATFORM IN_LIST WATCH_PLATFORMS))
    set(IS_IOS_WATCH_PLATFORM ON)
else()
    set(IS_IOS_WATCH_PLATFORM OFF)
endif()

# set C and C++ standards for all targets
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set build type-specific compilation flags
string(APPEND CMAKE_C_FLAGS_DEBUG     " -D_DEBUG")
string(APPEND CMAKE_CXX_FLAGS_DEBUG   " -D_DEBUG")
string(APPEND CMAKE_C_FLAGS_RELEASE   " -DNDEBUG")
string(APPEND CMAKE_CXX_FLAGS_RELEASE " -DNDEBUG")

# disable a set of commonly occurring warnings in gcc
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    string(APPEND CMAKE_C_FLAGS " -Wno-unused-result")
    string(APPEND CMAKE_CXX_FLAGS " -Wno-unused-result")
endif()

# disable a set of commonly occurring warnings in Visual C++ compiler
# also enable /EHsc required for exception handling
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    string(APPEND CMAKE_C_FLAGS " /wd4244 /wd4267 /wd4305 /EHsc")
    string(APPEND CMAKE_CXX_FLAGS " /wd4244 /wd4267 /wd4305 /EHsc")
endif()

# set default build type to Release (can be overridden by calling e.g. cmake -DCMAKE_BUILD_TYPE=Debug ..)
# allowed values: "Debug", "Release", "MinSizeRel", "RelWithDebInfo"
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message("Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE Release)
endif()

# set __WINDOWS directive on Windows platforms and disable some common warnings
if(WIN32)
    add_definitions(
        -D__WINDOWS
        -D_CRT_SECURE_NO_WARNINGS # disable warnings for using unsafe functions in Visual C++ runtime
        -D_CRT_NONSTDC_NO_DEPRECATE # disable warnings for non-standard POSIX functions like strdup
        -D_WINSOCK_DEPRECATED_NO_WARNINGS # disable warnings for using deprecated WinSock2 functions
    )
endif()

# set __ANDROID__ directive when compiling for Android
if(ANDROID_NDK)
    add_definitions(-D__ANDROID__)
endif()

# set __IOS__ directive when compiling for iOS
if(IS_IOS_PLATFORM)
    add_definitions(-D__IOS__)

    # set __IOS_WATCH__ directive when compiling for iOS Watch
    if(IS_IOS_WATCH_PLATFORM)
        add_definitions(-D__IOS_WATCH__)
    endif()

endif()

# set __OSX__ directive when compiling for Mac
if(APPLE)
    add_definitions(-D__OSX__)
endif()

add_definitions(${BUILD_FLAGS})

# update version information in src/include/core/git_version.hpp
if(IS_IOS_PLATFORM)
    # when using ios.toolchain.cmake, we need to use find_host_package to find Git on the host system
    find_host_package(Git)
else()
    find_package(Git)
endif()
if(GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --verify --quiet --short HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        ERROR_VARIABLE GIT_ERROR
        OUTPUT_VARIABLE VERSION_INFO_WC_REVISION
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT ${GIT_ERROR} EQUAL 0)
       set(VERSION_INFO_WC_REVISION "unknown")
    endif()
    execute_process(COMMAND ${GIT_EXECUTABLE} symbolic-ref --short HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        ERROR_VARIABLE GIT_ERROR
        OUTPUT_VARIABLE VERSION_INFO_BUILD_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT ${GIT_ERROR} EQUAL 0)
        set(VERSION_INFO_BUILD_BRANCH "unknown")
    endif()
else(GIT_FOUND)
    set(VERSION_INFO_WC_REVISION "unknown")
    set(VERSION_INFO_BUILD_BRANCH "unknown")
endif()
string(TIMESTAMP VERSION_INFO_BUILD_DATE UTC)
configure_file(src/include/core/git_version.hpp.in src/include/core/git_version.hpp)

# add targets in subdirectories
add_subdirectory(src/newmat)
add_subdirectory(progsrc/smileapi)

if(NOT IS_IOS_PLATFORM AND NOT IS_IOS_WATCH_PLATFORM)
    add_subdirectory(progsrc/smilextract)
endif()

# libopensmile target #########################################################

set(opensmile_SOURCES
    $<TARGET_OBJECTS:newmat>
    src/classifiers/libsvm/svm.cpp
    src/classifiers/libsvmliveSink.cpp
    src/classifiers/svmSink.cpp
    src/core/commandlineParser.cpp
    src/core/componentManager.cpp
    src/core/configManager.cpp
    src/core/dataMemory.cpp
    src/core/dataMemoryLevel.cpp
    src/core/dataProcessor.cpp
    src/core/dataReader.cpp
    src/core/dataSelector.cpp
    src/core/dataSink.cpp
    src/core/dataSource.cpp
    src/core/dataWriter.cpp
    src/core/exceptions.cpp
    src/core/nullSink.cpp
    src/core/smileCommon.cpp
    src/core/smileComponent.cpp
    src/core/smileLogger.cpp
    src/core/smileThread.cpp
    src/core/vecToWinProcessor.cpp
    src/core/vectorProcessor.cpp
    src/core/vectorTransform.cpp
    src/core/winToVecProcessor.cpp
    src/core/windowProcessor.cpp
    src/dsp/dbA.cpp
    src/dsp/signalGenerator.cpp
    src/dsp/smileResample.cpp
    src/dsp/specResample.cpp
    src/dsp/vadV1.cpp
    src/dsp/specScale.cpp
    src/dspcore/acf.cpp
    src/dspcore/amdf.cpp
    src/dspcore/contourSmoother.cpp
    src/dspcore/deltaRegression.cpp
    src/dspcore/fftmagphase.cpp
    src/dspcore/fftsg.c
    src/dspcore/framer.cpp
    src/dspcore/fullinputMean.cpp
    src/dspcore/fullturnMean.cpp
    src/dspcore/monoMixdown.cpp
    src/dspcore/preemphasis.cpp
    src/dspcore/transformFft.cpp
    src/dspcore/turnDetector.cpp
    src/dspcore/vectorMVN.cpp
    src/dspcore/vectorPreemphasis.cpp
    src/dspcore/windower.cpp
    src/examples/exampleSink.cpp
    src/examples/exampleSource.cpp
    src/examples/simpleMessageSender.cpp
    src/ffmpeg/ffmpegSource.cpp
    src/functionals/functionalComponent.cpp
    src/functionals/functionalCrossings.cpp
    src/functionals/functionalDCT.cpp
    src/functionals/functionalExtremes.cpp
    src/functionals/functionalLpc.cpp
    src/functionals/functionalMeans.cpp
    src/functionals/functionalMoments.cpp
    src/functionals/functionalOnset.cpp
    src/functionals/functionalPeaks.cpp
    src/functionals/functionalPeaks2.cpp
    src/functionals/functionalPercentiles.cpp
    src/functionals/functionalRegression.cpp
    src/functionals/functionalSamples.cpp
    src/functionals/functionalSegments.cpp
    src/functionals/functionalTimes.cpp
    src/functionals/functionalModulation.cpp
    src/functionals/functionals.cpp
    src/io/libsvmSink.cpp
    src/iocore/arffSink.cpp
    src/iocore/arffSource.cpp
    src/iocore/csvSink.cpp
    src/iocore/csvSource.cpp
    src/iocore/datadumpSink.cpp
    src/iocore/dataPrintSink.cpp
    src/iocore/htkSink.cpp
    src/iocore/htkSource.cpp
    src/iocore/externalSink.cpp
    src/iocore/externalSource.cpp
    src/iocore/externalAudioSource.cpp
    src/iocore/waveSink.cpp
    src/iocore/waveSinkCut.cpp
    src/iocore/waveSource.cpp
    src/lld/cens.cpp
    src/lld/chroma.cpp
    src/lld/formantLpc.cpp
    src/lld/formantSmoother.cpp
    src/lld/lpc.cpp
    src/lld/lsp.cpp
    src/lld/pitchDirection.cpp
    src/lld/pitchJitter.cpp
    src/lld/pitchShs.cpp
    src/lld/pitchSmootherViterbi.cpp
    src/lld/tonefilt.cpp
    src/lld/tonespec.cpp
    src/lld/harmonics.cpp
    src/lldcore/energy.cpp
    src/lldcore/intensity.cpp
    src/lldcore/melspec.cpp
    src/lldcore/mfcc.cpp
    src/lldcore/mzcr.cpp
    src/lldcore/pitchACF.cpp
    src/lldcore/pitchBase.cpp
    src/lldcore/pitchSmoother.cpp
    src/lldcore/plp.cpp
    src/lldcore/spectral.cpp
    src/other/maxIndex.cpp
    src/other/valbasedSelector.cpp
    src/other/vectorConcat.cpp
    src/other/vectorBinaryOperation.cpp
    src/other/vectorOperation.cpp
    src/other/externalMessageInterface.cpp
    src/portaudio/portaudioDuplex.cpp
    src/portaudio/portaudioSink.cpp
    src/portaudio/portaudioSource.cpp
    src/portaudio/portaudioWavplayer.cpp
    src/android/openslesSource.cpp
    src/rnn/rnn.cpp
    src/rnn/rnnProcessor.cpp
    src/rnn/rnnSink.cpp
    src/rnn/rnnVad2.cpp
    src/smileutil/smileUtil.c
    src/smileutil/smileUtilSpline.c
    src/smileutil/smileUtilCsv.cpp
    src/smileutil/zerosolve.cpp
    src/smileutil/JsonClasses.cpp
    src/video/openCVSource.cpp
)

if(IS_IOS_PLATFORM AND NOT IS_IOS_WATCH_PLATFORM)
    list(APPEND opensmile_SOURCES
        src/ios/iosRecorder.cpp
        src/ios/coreAudioSource.cpp
    )
endif()

if(STATIC_LINK)
    add_library(opensmile STATIC ${opensmile_SOURCES})
    target_compile_definitions(opensmile PUBLIC -D__STATIC_LINK)
else()
    add_library(opensmile SHARED ${opensmile_SOURCES})
endif()

target_include_directories(opensmile
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/include>
        $<INSTALL_INTERFACE:src/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/progsrc/include>
        $<INSTALL_INTERFACE:progsrc/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src/include> # for generated files (git_version.hpp)
)

target_link_libraries(opensmile
    PUBLIC
        Threads::Threads
)

# dl library needed for dynamic loading of libraries under Linux
if(NOT WIN32 AND NOT STATIC_LINK)
    target_link_libraries(opensmile
        PRIVATE
            dl
    )
endif()

if(WIN32)
    target_link_libraries(opensmile
        PRIVATE
            Ws2_32 # Winsock2 for networking under Windows
    )
endif()

if(ANDROID_NDK)
    find_library(log-lib log)
    target_link_libraries(opensmile PRIVATE ${log-lib})
endif(ANDROID_NDK)

if(IS_IOS_PLATFORM)
    # compile all C++ sources as Objective-C++
    foreach(source IN ITEMS ${opensmile_SOURCES})
        get_filename_component(extension "${source}" EXT)
        string(TOLOWER "${extension}" extension)
        if(extension STREQUAL ".cpp")
            set_source_files_properties(${source}
                PROPERTIES
                    COMPILE_FLAGS "-x objective-c++"
            )
        endif()
    endforeach()

    find_library(COREFOUNDATION CoreFoundation REQUIRED)
    find_library(AVFOUNDATION AVFoundation REQUIRED)
    find_library(AUDIOTOOLBOX AudioToolbox REQUIRED)

    target_link_libraries(opensmile
        PRIVATE
            ${COREFOUNDATION}
            ${AVFOUNDATION}
            ${AUDIOTOOLBOX}
    )
endif()

# allow linking of libopensmile into shared libraries
set_property(TARGET opensmile PROPERTY POSITION_INDEPENDENT_CODE ON)

if (MARCH_NATIVE AND NOT MSVC AND NOT ANDROID_NDK AND NOT IS_IOS_PLATFORM)
    target_compile_options(opensmile
        PRIVATE
            -march=native
            -mtune=native
    )
endif()

# profile-guided optimization
#
# 1. Compile with -DPROFILE_GENERATE=<path to profile folder>
# 2. Run SMILExtract on training samples. Profile information will be saved in the chosen folder.
# 3. If compiling using Clang: run
#        llvm-profdata merge -output=default.profdata *.profraw
#    in the profile folder.
# 4. Compile again with -DPROFILE_USE=<path to profile folder>
#
if(PROFILE_GENERATE)
    target_compile_options(opensmile
        PRIVATE
            "-fprofile-generate=${PROFILE_GENERATE}"
    )
    target_link_libraries(opensmile
        PRIVATE
            "-fprofile-generate=${PROFILE_GENERATE}"
    )
endif()
if(PROFILE_USE)
    target_compile_options(opensmile
        PRIVATE
            "-fprofile-use=${PROFILE_USE}"
    )
    target_link_libraries(opensmile
        PRIVATE
            "-fprofile-use=${PROFILE_USE}"
    )
endif()

# add own cmake folder with .cmake files to CMake module path  (used by find_package commands)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# pthread
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Portaudio
if(WITH_PORTAUDIO)
    find_package(portaudio REQUIRED)
    target_include_directories(opensmile PRIVATE ${PORTAUDIO_INCLUDE_DIRS})
    target_link_libraries(opensmile PRIVATE ${PORTAUDIO_LIBRARIES})
    target_compile_definitions(opensmile PRIVATE ${PORTAUDIO_DEFINITIONS})
    if((PORTAUDIO_VERSION EQUAL 19) OR (PORTAUDIO_VERSION GREATER 19))
        target_compile_definitions(opensmile PRIVATE -DHAVE_PORTAUDIO -DHAVE_PORTAUDIO_V19)
    else()
        target_compile_definitions(opensmile PRIVATE -DHAVE_PORTAUDIO)
    endif()
endif(WITH_PORTAUDIO)

# FFmpeg
if(WITH_FFMPEG)
    set(FFmpeg_FIND_COMPONENTS AVCODEC AVFORMAT AVUTIL) # specify required FFmpeg components
    find_package(FFmpeg REQUIRED)
    target_include_directories(opensmile PRIVATE ${FFMPEG_INCLUDE_DIRS})
    target_link_libraries(opensmile PRIVATE ${FFMPEG_LIBRARIES})
    target_compile_definitions(opensmile PRIVATE ${FFMPEG_DEFINITIONS})
    target_compile_definitions(opensmile PRIVATE -DHAVE_FFMPEG)
endif(WITH_FFMPEG)

# OpenSLES (Android-specific)
if(WITH_OPENSLES AND ANDROID_NDK)
    find_package(OpenSLES REQUIRED)
    target_include_directories(opensmile PRIVATE ${OPENSLES_INCLUDE_DIRS})
    target_link_libraries(opensmile PRIVATE ${OPENSLES_LIBRARIES})
    target_compile_definitions(opensmile PRIVATE ${OPENSLES_DEFINITIONS})
    target_compile_definitions(opensmile PRIVATE -DHAVE_OPENSLES)
endif(WITH_OPENSLES AND ANDROID_NDK)

# OpenCV
if(WITH_OPENCV)
    find_package(OpenCV REQUIRED)
    target_include_directories(opensmile PRIVATE ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(opensmile PRIVATE ${OpenCV_LIBS})
    target_compile_definitions(opensmile PRIVATE -DHAVE_OPENCV)
endif(WITH_OPENCV)

install(TARGETS opensmile
    DESTINATION lib
    EXPORT opensmile-targets)
install(EXPORT opensmile-targets
    FILE opensmile-config.cmake
    DESTINATION lib/cmake/opensmile)
