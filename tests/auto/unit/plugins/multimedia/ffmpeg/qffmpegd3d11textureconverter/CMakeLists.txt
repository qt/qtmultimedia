# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qffmpegd3d11textureconverter LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

if(NOT WIN32)
    return()
endif()

add_subdirectory(../shared shared)

qt_internal_multimedia_add_test(tst_qffmpegd3d11textureconverter
    SOURCES
        tst_qffmpegd3d11textureconverter.cpp
    LIBRARIES
        Qt::FFmpegMediaPluginImplPrivate
        Qt::MultimediaPrivate
        Qt::MultimediaTestLibPrivate
        Qt::GuiPrivate
        Qt::CorePrivate
        ffmpeg_test_support
)

if(TARGET qt_plugins_qtbase)
    add_dependencies(tst_qffmpegd3d11textureconverter qt_plugins_qtbase)
endif()
