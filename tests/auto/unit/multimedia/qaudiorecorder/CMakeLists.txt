# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qaudiorecorder Test:
#####################################################################

qt_internal_multimedia_add_test(tst_qaudiorecorder
    SOURCES
        tst_qaudiorecorder.cpp
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MockMultimediaPlugin
)
