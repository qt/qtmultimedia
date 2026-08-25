# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qwindowscamera Test:
#####################################################################

if(NOT WIN32)
    return()
endif()

qt_internal_multimedia_add_test(tst_qwindowscamera
    SOURCES
        tst_qwindowscamera.cpp
    LIBRARIES
        Qt::FFmpegMediaPluginImplPrivate
        Qt::MultimediaPrivate
)
