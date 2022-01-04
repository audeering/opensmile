cmake_minimum_required(VERSION 3.5.1)
project(openSMILEplugin)

# set C and C++ standards for all targets
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
