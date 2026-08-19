# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qffmpegsubtitledecoder Test:
#####################################################################

if(QT_FEATURE_ffmpeg_stubs)
    message(WARNING "tst_qffmpegsubtitledecoder can not run with stubbed ffmpeg because of QTBUG-133914")
    return()
endif()

qt_internal_multimedia_add_test(tst_qffmpegsubtitledecoder
    SOURCES
        tst_qffmpegsubtitledecoder.cpp
    LIBRARIES
        Qt::FFmpegMediaPluginImplPrivate
        Qt::MultimediaPrivate
)
