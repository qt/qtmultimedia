# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsoundeffect Test:
#####################################################################

qt_internal_add_test(tst_qsoundeffect
    SOURCES
        tst_qsoundeffect.cpp
    LIBRARIES
        Qt::Core
        Qt::MultimediaPrivate
    TESTDATA
        "test.wav"
        "test_corrupted.wav"
        "test_tone.wav"
)
