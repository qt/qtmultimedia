# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qcamera Test:
#####################################################################

qt_internal_add_test(tst_multimedia_qcamera
    SOURCES
        tst_qcamera.cpp
        ../../../shared/mediabackendutils.h
    INCLUDE_DIRECTORIES
        ../../mockbackend
        ../../../shared/
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MockMultimediaPlugin
)
