# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_multimedia_qcamera LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_multimedia_add_test(tst_multimedia_qcamera
    SOURCES
        tst_qcamera.cpp
    LIBRARIES
        Qt::Core
        Qt::MultimediaPrivate
        Qt::MockMultimediaPlugin
        Qt::MultimediaTestLibPrivate
)
