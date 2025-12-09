# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qscreencapturebackend Test:
#####################################################################

qt_internal_add_test(tst_qscreencapturebackend
    SOURCES
        tst_qscreencapturebackend.cpp
        ../../shared/mediabackendutils.h
    INCLUDE_DIRECTORIES
        ../../shared/
    LIBRARIES
        Qt::Multimedia
        Qt::MultimediaPrivate
        Qt::Gui
        Qt::Widgets
)


