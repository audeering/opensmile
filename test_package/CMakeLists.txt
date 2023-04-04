cmake_minimum_required(VERSION 3.10.2)
project(smileapi-tests CXX)

find_package(opensmile REQUIRED)

add_executable(smileapi-test smileapi_test.cpp)
target_link_libraries(smileapi-test PRIVATE opensmile::opensmile)

enable_testing()
add_test(NAME smileapi-test
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMAND smileapi-test)
