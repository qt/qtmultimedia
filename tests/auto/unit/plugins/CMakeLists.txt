# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(multimedia)
if(QT_FEATURE_libcamera)
    add_subdirectory(libcamera)
endif()
