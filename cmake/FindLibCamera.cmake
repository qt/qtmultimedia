#.rst:
# FindLibCamera
# -------
#
# Try to find libcamera on a Unix system.
#
# This will define the following variables:
#
# ``LibCamera_FOUND``
#     True if (the requested version of) libcamera is available
# ``LibCamera_VERSION``
#     The version of libcamera
# ``LibCamera_LIBRARIES``
#     This can be passed to target_link_libraries() instead of the ``LibCamera::LibCamera``
#     target
# ``LibCamera_INCLUDE_DIRS``
#     This should be passed to target_include_directories() if the target is not
#     used for linking
#
# If ``LibCamera_FOUND`` is TRUE, it will also define the following imported target:
#
# ``LibCamera::LibCamera``
#     The libcamera library
#
# In general we recommend using the imported target, as it is easier to use.

# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

find_package(PkgConfig QUIET)

if(PkgConfig_FOUND)
    pkg_search_module(PKG_LibCamera QUIET libcamera)
endif()

set(LibCamera_VERSION "${PKG_LibCamera_VERSION}")

find_path(LibCamera_INCLUDE_DIRS
    NAMES
        libcamera/libcamera.h
    HINTS
        ${PKG_LibCamera_INCLUDE_DIRS}
)

find_library(LibCamera_LIBRARIES
    NAMES
        camera
    HINTS
        ${PKG_LibCamera_LIBRARY_DIRS}
)

find_library(LibCameraBase_LIBRARIES
    NAMES
        camera-base
    HINTS
        ${PKG_LibCamera_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibCamera
    FOUND_VAR
        LibCamera_FOUND
    REQUIRED_VARS
        LibCamera_LIBRARIES
        LibCameraBase_LIBRARIES
        LibCamera_INCLUDE_DIRS
    VERSION_VAR
        LibCamera_VERSION
)

if(LibCamera_FOUND AND NOT TARGET LibCamera::LibCamera)
    add_library(LibCamera::LibCamera UNKNOWN IMPORTED)
    set_target_properties(LibCamera::LibCamera PROPERTIES
        IMPORTED_LOCATION "${LibCamera_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${LibCamera_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES "${LibCameraBase_LIBRARIES}"
    )
endif()

mark_as_advanced(LibCamera_LIBRARIES LibCameraBase_LIBRARIES LibCamera_INCLUDE_DIRS)

include(FeatureSummary)
set_package_properties(LibCamera PROPERTIES
    URL "https://libcamera.org"
    DESCRIPTION "libcamera - support library for complex camera devices"
)
