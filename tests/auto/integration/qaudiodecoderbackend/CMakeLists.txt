# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qaudiodecoderbackend.pro.

#####################################################################
## tst_qaudiodecoderbackend Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    testdata/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qaudiodecoderbackend
    SOURCES
        ../shared/mediafileselector.h
        tst_qaudiodecoderbackend.cpp
    INCLUDE_DIRECTORIES
        ../../../../src/multimedia/audio
    LIBRARIES
        Qt::Gui
        Qt::Multimedia
        Qt::MultimediaPrivate
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qaudiodecoderbackend CONDITION boot2qt
    DEFINES
        WAV_SUPPORT_NOT_FORCED
)
