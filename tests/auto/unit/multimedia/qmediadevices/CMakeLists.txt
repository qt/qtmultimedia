# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_multimedia_add_test(tst_qmediadevices
    SOURCES
        tst_qmediadevices.cpp
    LIBRARIES
        Qt::MultimediaPrivate
        Qt::MockMultimediaPlugin
)
