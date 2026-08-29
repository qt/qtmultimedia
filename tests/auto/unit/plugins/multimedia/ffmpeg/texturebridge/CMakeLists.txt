# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_texturebridge LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

if(NOT WIN32)
    return()
endif()

qt_internal_multimedia_add_test(tst_texturebridge
    LINK_FFMPEG_STUBS
    SOURCES
        devicecontext.cpp devicecontext.h
        tst_texturebridge.cpp
    LIBRARIES
        Qt::FFmpegMediaPluginImplPrivate
        Qt::MultimediaPrivate
        Qt::CorePrivate
)
