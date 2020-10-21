find_path(LIBSENT_INCLUDE_DIR
    NAMES
        sent/mfcc.h
    PATHS
        /usr/include
        /usr/local/include
        /opt/local/include
        /sw/include
)

find_path(LIBJULIUS_INCLUDE_DIR
    NAMES
        julius/juliuslib.h
    PATHS
        /usr/include
        /usr/local/include
        /opt/local/include
        /sw/include
)

find_library(LIBSENT_LIBRARY NAMES sent libsent)
find_library(LIBJULIUS_LIBRARY NAMES julius libjulius)

if(LIBSENT_INCLUDE_DIR AND LIBJULIUS_INCLUDE_DIR AND LIBSENT_LIBRARY AND LIBJULIUS_LIBRARY)
    add_library(Sent UNKNOWN IMPORTED)
    set_target_properties(Sent PROPERTIES 
        INTERFACE_INCLUDE_DIRECTORIES "${LIBSENT_INCLUDE_DIR}"
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
        IMPORTED_LOCATION "${LIBSENT_LIBRARY}"
        INTERFACE_COMPILE_DEFINITIONS "EXTERNAL_FV"
    )

    add_library(Julius UNKNOWN IMPORTED)
    set_target_properties(Julius PROPERTIES 
        INTERFACE_INCLUDE_DIRECTORIES "${LIBJULIUS_INCLUDE_DIR}"
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
        IMPORTED_LOCATION "${LIBJULIUS_LIBRARY}"
        INTERFACE_COMPILE_DEFINITIONS "EXTERNAL_FV"
        INTERFACE_LINK_LIBRARIES Sent # libjulius depends on libsent
    )

    set(Julius_FOUND TRUE)
endif()

if(Julius_FOUND)
    if (NOT Julius_FIND_QUIETLY)
        message(STATUS "Found Julius: libjulius: ${LIBJULIUS_LIBRARY} (include dir: ${LIBJULIUS_INCLUDE_DIR}), libsent: ${LIBSENT_LIBRARY} (include dir: ${LIBSENT_INCLUDE_DIR})")
    endif (NOT Julius_FIND_QUIETLY)
else(Julius_FOUND)
    if (Julius_FIND_REQUIRED)
        message(FATAL_ERROR "Could not find Julius. Make sure libjulius and its dependency libsent are installed. Set LIBSENT_INCLUDE_DIR, LIBJULIUS_INCLUDE_DIR, LIBSENT_LIBRARY and LIBJULIUS_LIBRARY appropriately.")
    endif(Julius_FIND_REQUIRED)
endif(Julius_FOUND)
