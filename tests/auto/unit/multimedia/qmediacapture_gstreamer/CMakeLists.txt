# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qmediacapture_gstreamer Test:
#####################################################################

qt_internal_add_test(tst_qmediacapture_gstreamer
    SOURCES
        tst_qmediacapture_gstreamer.cpp
    LIBRARIES
        Qt::Multimedia
        Qt::MultimediaPrivate
        Qt::GstreamerMediaPluginImplPrivate
        Qt::MultimediaTestLibPrivate
)
