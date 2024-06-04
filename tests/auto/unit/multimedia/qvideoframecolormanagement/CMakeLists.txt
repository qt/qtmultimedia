# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    testdata/*)
list(APPEND testdata_resource_files ${test_data_glob})

qt_internal_add_test(tst_qvideoframecolormanagement
    SOURCES
        tst_qvideoframecolormanagement.cpp
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
    BUILTIN_TESTDATA
    TESTDATA ${testdata_resource_files}
)
