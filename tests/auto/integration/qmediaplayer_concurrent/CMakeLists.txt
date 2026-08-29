# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qmediaplayer_concurrent LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_multimedia_add_test(tst_qmediaplayer_concurrent
    NEEDS_MEDIA_BACKEND_PLUGINS
    SOURCES
        tst_qmediaplayer_concurrent.cpp
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MultimediaTestLibPrivate
    TESTDATA "3colors_with_sound_1s.mp4"
    BUILTIN_TESTDATA
)
