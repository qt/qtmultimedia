# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_test(tst_qsoundeffect_concurrent
    SOURCES
        tst_qsoundeffect_concurrent.cpp
    LIBRARIES
        Qt::Core
        Qt::MultimediaPrivate
    TESTDATA "double-drop.wav"
    BUILTIN_TESTDATA
)
