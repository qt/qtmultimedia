# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qsoundeffect.pro.

#####################################################################
## tst_qsoundeffect Test:
#####################################################################

# Collect test data
list(APPEND test_data "test.wav")

qt_internal_add_test(tst_qsoundeffect
    SOURCES
        tst_qsoundeffect.cpp
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
    TESTDATA ${test_data}
)

# Resources:
set(resources_resource_files
    "test.wav"
    "test_corrupted.wav"
    "test_tone.wav"
)

qt_internal_add_resource(tst_qsoundeffect "resources"
    PREFIX
        "/"
    FILES
        ${resources_resource_files}
)


## Scopes:
#####################################################################

qt_internal_extend_target(tst_qsoundeffect CONDITION UNIX AND NOT APPLE AND NOT QT_FEATURE_pulseaudio
    DEFINES
        QT_MULTIMEDIA_QMEDIAPLAYER
)
