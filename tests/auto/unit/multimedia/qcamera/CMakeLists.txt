# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qcamera Test:
#####################################################################

qt_internal_add_test(tst_multimedia_qcamera
    SOURCES
        tst_qcamera.cpp
    INCLUDE_DIRECTORIES
        ../../mockbackend
    LIBRARIES
        Qt::Core
        Qt::MultimediaPrivate
        Qt::MockMultimediaPlugin
        Qt::MultimediaTestLibPrivate
)
