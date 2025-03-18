# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qmediaplayer_gstreamer Test:
#####################################################################

qt_internal_add_test(tst_qmediaplayer_gstreamer
    SOURCES
        tst_qmediaplayer_gstreamer.cpp
        tst_qmediaplayer_gstreamer.h
    LIBRARIES
        Qt::MultimediaPrivate
        Qt::GstreamerMediaPluginImplPrivate
        Qt::MultimediaTestLibPrivate
    TESTDATA
        "testdata/color_matrix.mp4"
    BUILTIN_TESTDATA
)
