# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from QMediaRecorder.pro.

#####################################################################
## tst_QMediaRecorder Test:
#####################################################################

qt_internal_add_test(tst_qmediarecorder
    SOURCES
        tst_qmediarecorder.cpp
    INCLUDE_DIRECTORIES
        ../../mockbackend
    LIBRARIES
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}
        Qt::Gui
        Qt::MultimediaPrivate
        QtMultimediaMockBackend
)
