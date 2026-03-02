# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_test(tst_qspatialaudio
    SOURCES
        tst_qspatialaudio.cpp
    LIBRARIES
        Qt::Core
        Qt::SpatialAudio
        Qt::Multimedia
    TESTDATA
        "test.wav"
)
