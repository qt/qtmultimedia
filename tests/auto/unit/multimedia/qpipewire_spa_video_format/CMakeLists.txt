# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qpipewire_spa_video_format LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

if(NOT QT_FEATURE_pipewire)
    return()
endif()

qt_find_package(PipeWire PROVIDED_TARGETS PipeWire::PipeWire MODULE_NAME multimedia QMAKE_LIB pipewire)

qt_internal_multimedia_add_test(tst_qpipewire_spa_video_format
    SOURCES
        tst_qpipewire_spa_video_format.cpp
    LIBRARIES
        Qt::MultimediaPrivate
        PipeWire::PipeWire
)
