# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(QT_FEATURE_ffmpeg)
    add_subdirectory(ffmpeg)
endif()

if(QT_FEATURE_gstreamer)
    add_subdirectory(gstreamer)
endif()

if(QT_FEATURE_native_windows_backend)
    add_subdirectory(windows)
endif()
