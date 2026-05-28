# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_test(tst_multimedia_shutdown
    SOURCES
        tst_shutdown.cpp
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::MultimediaPrivate
    BUILTIN_TESTDATA
    TESTDATA
        testdata/test.mp4
        testdata/test.wav
)
