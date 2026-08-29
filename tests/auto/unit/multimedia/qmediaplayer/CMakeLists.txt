# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qmediaplayer LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_multimedia_add_test(tst_qmediaplayer
    SOURCES
        tst_qmediaplayer.cpp
    LIBRARIES
        Qt::Core
        Qt::MultimediaPrivate
        Qt::MultimediaTestLibPrivate
        Qt::MockMultimediaPlugin
)

# Resources:
set(testdata_resource_files
    "testdata/nokia-tune.mp3"
)

qt_internal_add_resource(tst_qmediaplayer "testdata"
    PREFIX
        "/"
    FILES
        ${testdata_resource_files}
)

