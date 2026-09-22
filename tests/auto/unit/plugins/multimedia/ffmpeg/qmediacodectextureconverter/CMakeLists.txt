# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qmediacodectextureconverter LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

if(NOT ANDROID)
    return()
endif()

qt_internal_multimedia_add_test(tst_qmediacodectextureconverter
    LINK_FFMPEG_STUBS
    SOURCES
        tst_qmediacodectextureconverter.cpp
    LIBRARIES
        Qt::FFmpegMediaPluginImplPrivate
        Qt::MultimediaPrivate
        Qt::MultimediaTestLibPrivate
        Qt::GuiPrivate
        Qt::CorePrivate
    BUILTIN_TESTDATA
    TESTDATA
        testdata/one_red_frame.mp4
        testdata/one_red_frame_p010.mp4
        testdata/colors.mp4
)

if(TARGET qt_plugins_qtbase)
    add_dependencies(tst_qmediacodectextureconverter qt_plugins_qtbase)
endif()
