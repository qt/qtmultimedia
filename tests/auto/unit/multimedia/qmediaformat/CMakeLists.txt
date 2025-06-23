# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_test(tst_qmediaformat
    SOURCES
        tst_qmediaformat.cpp
    INCLUDE_DIRECTORIES
        ../../mockbackend
    LIBRARIES
        Qt::Core
        Qt::MultimediaPrivate
        Qt::MockMultimediaPlugin
)
