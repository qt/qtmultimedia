# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qaudiodecoder LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_multimedia_add_test(tst_qaudiodecoder
    SOURCES
        tst_qaudiodecoder.cpp
    LIBRARIES
        Qt::Multimedia
        Qt::MultimediaPrivate
        Qt::MockMultimediaPlugin
)

qt_internal_add_resource(tst_qaudiodecoder "testdata"
    PREFIX
        "/"
    FILES
        "testdata/dummy.mp3"
)
