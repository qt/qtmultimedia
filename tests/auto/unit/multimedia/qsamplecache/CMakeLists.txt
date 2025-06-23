# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsamplecache Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data
    CONFIGURE_DEPENDS
    RELATIVE
        ${CMAKE_CURRENT_SOURCE_DIR}
    testdata/*)

qt_internal_add_test(tst_qsamplecache
    SOURCES
        tst_qsamplecache.cpp
    LIBRARIES
        Qt::MultimediaPrivate
    TESTDATA ${test_data}
)
