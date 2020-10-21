cmake_minimum_required(VERSION 3.5.1)
project(openSMILEplugin)

# set C++ standard to version 11 for all targets
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT WIN32)
    add_definitions(-DHAVE_UNISTD_H -DHAVE_PTHREAD)
endif()

add_definitions(${BUILD_FLAGS})

add_library(exampleplugin SHARED 
    pluginMain.cpp
    plug_exampleSink.cpp
)

target_include_directories(exampleplugin
    PRIVATE
        .
)

add_subdirectory(.. opensmile)

target_link_libraries(exampleplugin
    opensmile
)
