# SMILEapi target #########################################################
cmake_minimum_required(VERSION 3.5.1)

option(SMILEAPI_STATIC_LINK "Creating a static library" OFF)

if(SMILEAPI_STATIC_LINK)
    add_library(SMILEapi STATIC
        SMILEapi.cpp)
else()
    add_library(SMILEapi SHARED
        SMILEapi.cpp
        SMILEapi.def)
endif()

# allow linking of SMILEapi into shared libraries
set_property(TARGET SMILEapi PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(SMILEapi
    PRIVATE
        ../../src/include
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../progsrc/include>
        $<INSTALL_INTERFACE:include>
)

set_target_properties(SMILEapi
    PROPERTIES
        PUBLIC_HEADER
            "../include/smileapi/SMILEapi.h"
)

target_link_libraries(SMILEapi
    PRIVATE
        opensmile
)

install(TARGETS SMILEapi
    EXPORT SMILEapi-targets
    RUNTIME DESTINATION
        bin
    LIBRARY DESTINATION
        lib
    ARCHIVE DESTINATION
        lib
    PUBLIC_HEADER DESTINATION
        "include/smileapi"
)
install(EXPORT SMILEapi-targets
    FILE smileapi-config.cmake
    DESTINATION lib/cmake/SMILEapi
)
