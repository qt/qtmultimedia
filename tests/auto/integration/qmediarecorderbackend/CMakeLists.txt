# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_test(tst_qmediarecorderbackend
    SOURCES
        tst_qmediarecorderbackend.cpp
        ../shared/audiogenerationutils.h
        ../../shared/mediabackendutils.h
    INCLUDE_DIRECTORIES
        ../../shared/
    LIBRARIES
        Qt::MultimediaPrivate
)
