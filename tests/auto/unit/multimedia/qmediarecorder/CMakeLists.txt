# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_QMediaRecorder Test:
#####################################################################

qt_internal_multimedia_add_test(tst_qmediarecorder
    SOURCES
        tst_qmediarecorder.cpp
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MockMultimediaPlugin
)
