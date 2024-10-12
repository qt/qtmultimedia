# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qmediaframeinputsbackend Test:
#####################################################################

qt_internal_add_test(tst_qmediaframeinputsbackend
    SOURCES
        tst_qmediaframeinputsbackend.cpp tst_qmediaframeinputsbackend.h
    LIBRARIES
        Qt::Multimedia
        Qt::MultimediaPrivate
        Qt::Gui
        Qt::MultimediaTestLibPrivate
)


