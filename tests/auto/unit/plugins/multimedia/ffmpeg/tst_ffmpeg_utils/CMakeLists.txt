# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_ffmpeg_utils Test:
#####################################################################

qt_internal_multimedia_add_test(tst_ffmpeg_utils
    LINK_FFMPEG_STUBS
    SOURCES
        tst_ffmpeg_utils.cpp
    LIBRARIES
        Qt::FFmpegMediaPluginImplPrivate
        Qt::MultimediaPrivate
)
