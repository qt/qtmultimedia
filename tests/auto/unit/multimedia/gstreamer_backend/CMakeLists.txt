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
        Qt::GstreamerMediaPluginImplPrivate

    BUILTIN_TESTDATA
    TESTDATA
        "metadata_test_file.mp4"
        "color_matrix_90_deg_clockwise.mp4"
        "color_matrix_90_deg_clockwise.m2v"
)
