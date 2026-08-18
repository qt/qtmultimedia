# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qffmpegmath Test:
#####################################################################

qt_internal_multimedia_add_test(tst_qffmpegmath
    LINK_FFMPEG_STUBS
    SOURCES
        tst_qffmpegmath.cpp
    LIBRARIES
        Qt::FFmpegMediaPluginImplPrivate
        Qt::MultimediaPrivate
)
