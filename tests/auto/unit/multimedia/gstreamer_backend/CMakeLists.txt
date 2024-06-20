# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_gstreamer_backend Test:
#####################################################################

qt_internal_add_test(tst_gstreamer_backend
    SOURCES
        tst_gstreamer_backend.cpp
        tst_gstreamer_backend.h
    LIBRARIES
        Qt::MultimediaPrivate
        Qt::QGstreamerMediaPluginImplPrivate
)
