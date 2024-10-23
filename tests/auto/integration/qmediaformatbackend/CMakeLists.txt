# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_test(tst_qmediaformatbackend
    SOURCES
        tst_qmediaformatbackend.cpp
    LIBRARIES
        Qt::MultimediaPrivate
        Qt::MultimediaTestLibPrivate
)
