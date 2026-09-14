# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(shared)

add_subdirectory(qffmpegmath)
add_subdirectory(qffmpegvideoencoderutils)
add_subdirectory(qffmpegsubtitledecoder)
add_subdirectory(tst_ffmpeg_utils)

if(APPLE)
    add_subdirectory(qvideotoolboxtextureconverter)
endif()

if(WIN32)
    add_subdirectory(texturebridge)
    add_subdirectory(qwindowscamera)
endif()

if (LINUX AND QT_FEATURE_vaapi)
    add_subdirectory(qvaapitextureconverter)
endif()
