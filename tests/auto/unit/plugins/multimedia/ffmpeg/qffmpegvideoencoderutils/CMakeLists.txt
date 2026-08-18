# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qffmpegvideoencoderutils Test:
#####################################################################

qt_internal_multimedia_add_test(tst_qffmpegvideoencoderutils
    LINK_FFMPEG_STUBS
    SOURCES
        tst_qffmpegvideoencoderutils.cpp
    LIBRARIES
        Qt::FFmpegMediaPluginImplPrivate
        Qt::MultimediaPrivate
)
