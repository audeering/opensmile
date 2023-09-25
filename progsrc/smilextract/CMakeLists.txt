# SMILExtract target #########################################################
cmake_minimum_required(VERSION 3.5.1)

add_executable(SMILExtract
    SMILExtract.cpp 
)

target_link_libraries(SMILExtract
    PRIVATE
        opensmile
)

install(TARGETS SMILExtract
    DESTINATION bin
    EXPORT SMILExtract-targets)
install(EXPORT SMILExtract-targets
    FILE SMILExtract-config.cmake
    DESTINATION lib/cmake/SMILExtract)