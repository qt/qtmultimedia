# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qmediaplaylist.pro.

#####################################################################
## tst_qmediaplaylist Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    testdata/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qmediaplaylist
    SOURCES
        tst_qmediaplaylist.cpp
    LIBRARIES
        Qt::Core
        Qt::MultimediaPrivate
    TESTDATA ${test_data}
)
