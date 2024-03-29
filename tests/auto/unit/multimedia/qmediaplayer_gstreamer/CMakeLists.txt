# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qmediaplayer_gstreamer Test:
#####################################################################

qt_internal_add_test(tst_qmediaplayer_gstreamer
    SOURCES
        tst_qmediaplayer_gstreamer.cpp
    LIBRARIES
        Qt::MultimediaPrivate
        Qt::QGstreamerMediaPluginPrivate
)
