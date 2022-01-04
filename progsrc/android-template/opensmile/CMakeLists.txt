cmake_minimum_required(VERSION 3.5.1)
project(openSMILE)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message("Generating with COMPILESOURCE flavor.")

add_subdirectory(${OPENSMILE_ROOT} ${CMAKE_CURRENT_BINARY_DIR}/opensmile)

add_library(opensmile_jni MODULE ${CMAKE_CURRENT_SOURCE_DIR}/opensmile_wrap.cxx ${CMAKE_CURRENT_SOURCE_DIR}/opensmile_wrap.h)
find_library(
        log-lib
        log)
target_link_libraries(opensmile_jni
        PRIVATE
        SMILEapi ${log-lib})