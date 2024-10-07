# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

file(GLOB_RECURSE test_data_glob
    RELATIVE
        ${CMAKE_CURRENT_SOURCE_DIR}
    testdata/**
)

list(APPEND testdata_resource_files ${test_data_glob})

qt_internal_add_test(tst_qmediaplayerformatsupport
    SOURCES
        tst_qmediaplayerformatsupport.cpp
        ../../shared/mediabackendutils.h
    INCLUDE_DIRECTORIES
        ../../shared/
    LIBRARIES
        Qt::Core
        Qt::MultimediaPrivate
    TESTDATA
        ${testdata_resource_files}
)

qt_internal_add_resource(tst_qmediaplayerformatsupport "testdata"
    PREFIX
        "/"
    FILES
        ${testdata_resource_files}
)
