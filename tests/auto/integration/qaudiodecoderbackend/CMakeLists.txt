# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qaudiodecoderbackend Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data
    CONFIGURE_DEPENDS
    RELATIVE
        ${CMAKE_CURRENT_SOURCE_DIR}
    testdata/*)

qt_internal_add_test(tst_qaudiodecoderbackend
    SOURCES
        tst_qaudiodecoderbackend.cpp
        ../shared/mediafileselector.h
    INCLUDE_DIRECTORIES
        ../shared/
        ../../../../src/multimedia/audio
    LIBRARIES
        Qt::Gui
        Qt::Multimedia
        Qt::MultimediaPrivate
        Qt::MultimediaTestLibPrivate
    TESTDATA ${test_data}
)
