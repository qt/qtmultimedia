# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qvideoframecolormanagement LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()


file(GLOB_RECURSE test_data
    CONFIGURE_DEPENDS
    RELATIVE
        ${CMAKE_CURRENT_SOURCE_DIR}
    testdata/*)

qt_internal_multimedia_add_test(tst_qvideoframecolormanagement
    SOURCES
        tst_qvideoframecolormanagement.cpp
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MultimediaTestLibPrivate
    BUILTIN_TESTDATA
    TESTDATA ${test_data}
)
