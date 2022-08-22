# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qwavedecoder.pro.

#####################################################################
## tst_qwavedecoder Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qwavedecoder
    SOURCES
        tst_qwavedecoder.cpp
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::Network
    TESTDATA ${test_data}
)
