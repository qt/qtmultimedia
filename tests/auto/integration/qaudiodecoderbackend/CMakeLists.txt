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
    INCLUDE_DIRECTORIES
        ../../../../src/multimedia/audio
    LIBRARIES
        Qt::Core
        Qt::Multimedia
        Qt::MultimediaPrivate
        Qt::MultimediaTestLibPrivate
    TESTDATA ${test_data}
)
