# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qffmpegmath Test:
#####################################################################

if(QT_FEATURE_ffmpeg_stubs)
    message(WARNING "tst_qffmpegmath can not run with stubbed ffmpeg because of QTBUG-133914")
    return()
endif()

qt_internal_add_test(tst_qffmpegmath
    SOURCES
        tst_qffmpegmath.cpp
    LIBRARIES
        Qt::FFmpegMediaPluginImplPrivate
        Qt::MultimediaPrivate
)
