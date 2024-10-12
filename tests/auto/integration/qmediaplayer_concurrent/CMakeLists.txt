# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_test(tst_qmediaplayer_concurrent
    SOURCES
        tst_qmediaplayer_concurrent.cpp
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MultimediaTestLibPrivate
    TESTDATA "3colors_with_sound_1s.mp4"
    BUILTIN_TESTDATA
)
