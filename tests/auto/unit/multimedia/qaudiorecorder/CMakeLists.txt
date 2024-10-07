# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qaudiorecorder Test:
#####################################################################

qt_internal_add_test(tst_qaudiorecorder
    SOURCES
        tst_qaudiorecorder.cpp
    INCLUDE_DIRECTORIES
        ../../mockbackend
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MockMultimediaPlugin
)
