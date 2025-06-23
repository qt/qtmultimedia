# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qwavedecoder Test:
#####################################################################

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
        Qt::Core
        Qt::MultimediaPrivate
        Qt::Network
        Qt::MultimediaTestLibPrivate
    TESTDATA ${test_data}
)
