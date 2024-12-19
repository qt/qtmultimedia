# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qwavedecoder Test:
#####################################################################

if(NOT TARGET Qt::Network)
    message(NOTICE "Skipping the build of tst_qwavedecoder as the condition \"TARGET Qt::Network\" is not met.")
    return()
endif()

# Collect test data
file(GLOB_RECURSE test_data
    CONFIGURE_DEPENDS
    RELATIVE
        ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)

qt_internal_add_test(tst_qwavedecoder
    SOURCES
        tst_qwavedecoder.cpp
        data/gendata.sh
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::Network
        Qt::MultimediaTestLibPrivate
    TESTDATA ${test_data}
)
