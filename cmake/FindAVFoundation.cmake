# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

find_library(AVFoundation_LIBRARY NAMES AVFoundation)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AVFoundation DEFAULT_MSG AVFoundation_LIBRARY)

if(AVFoundation_FOUND AND NOT TARGET AVFoundation::AVFoundation)
    add_library(AVFoundation::AVFoundation INTERFACE IMPORTED)
    set_target_properties(AVFoundation::AVFoundation PROPERTIES
                          INTERFACE_LINK_LIBRARIES "${AVFoundation_LIBRARY}")
endif()

mark_as_advanced(AVFoundation_LIBRARY)
