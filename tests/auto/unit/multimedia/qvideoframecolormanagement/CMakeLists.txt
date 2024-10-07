# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


file(GLOB_RECURSE test_data
    CONFIGURE_DEPENDS
    RELATIVE
        ${CMAKE_CURRENT_SOURCE_DIR}
    testdata/*)

qt_internal_add_test(tst_qvideoframecolormanagement
    SOURCES
        tst_qvideoframecolormanagement.cpp
        ../../../shared/mediabackendutils.h
    INCLUDE_DIRECTORIES
        ../../../shared
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
    BUILTIN_TESTDATA
    TESTDATA ${test_data}
)
