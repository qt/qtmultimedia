qt_internal_add_3rdparty_library(BundledTLSF
    STATIC
    SKIP_AUTOMOC
    SOURCES
        tlsf.h tlsf.cpp
    PUBLIC_INCLUDE_DIRECTORIES
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    ATTRIBUTION_FILE_DIR_PATHS
        .
    LIBRARIES
        Qt::Core
)
