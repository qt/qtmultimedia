# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qffmpegvideoencoderutils Test:
#####################################################################

if(NOT WIN32)
    return()
endif()

qt_internal_add_test(tst_texturebridge
    SOURCES
        devicecontext.cpp devicecontext.h
        tst_texturebridge.cpp
    LIBRARIES
        Qt::FFmpegMediaPluginImplPrivate
        Qt::MultimediaPrivate
        Qt::CorePrivate
)
