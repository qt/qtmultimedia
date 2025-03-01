# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qffmpegvideoencoderutils Test:
#####################################################################

if(LINUX OR ANDROID)
    message(WARNING "tst_qffmpegmath can not run on Linux or Android because of QTBUG-133914")
    return()
endif()

qt_internal_add_test(tst_qffmpegmath
    SOURCES
        tst_qffmpegmath.cpp
    LIBRARIES
        Qt::FFmpegMediaPluginImplPrivate
        Qt::MultimediaPrivate
)
