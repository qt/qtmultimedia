# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qffmpegvideoencoderutils Test:
#####################################################################

qt_internal_add_test(tst_qffmpegvideoencoderutils
    SOURCES
        tst_qffmpegvideoencoderutils.cpp
    LIBRARIES
        Qt::FFmpegMediaPluginImplPrivate
        Qt::MultimediaPrivate
)
