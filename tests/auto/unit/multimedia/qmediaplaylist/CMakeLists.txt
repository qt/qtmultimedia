# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qmediaplaylist LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Generated from qmediaplaylist.pro.

qt_internal_multimedia_add_test(tst_qmediaplaylist
    SOURCES
        tst_qmediaplaylist.cpp
    LIBRARIES
        Qt::Core
        Qt::MultimediaPrivate
    TESTDATA
        testdata/empty.pls
        testdata/test.m3u
        testdata/test.pls
        testdata/testfile
        testdata/testfile2#suffix
        testdata/totem-pl-example.pls
)
