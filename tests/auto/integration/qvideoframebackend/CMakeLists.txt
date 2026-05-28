# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qvideoframebackend Test:
#####################################################################

qt_internal_add_test(tst_qvideoframebackend
    SOURCES
        tst_qvideoframebackend.cpp
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MultimediaTestLibPrivate
    BUILTIN_TESTDATA
    TESTDATA
        testdata/colors.mp4
        testdata/one_red_frame.mp4
)
