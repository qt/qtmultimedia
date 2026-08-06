# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qdmabuftextureimporter LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

if(NOT LINUX OR NOT QT_FEATURE_linux_dmabuf)
    return()
endif()

qt_internal_multimedia_add_test(tst_qdmabuftextureimporter
    SOURCES
        tst_qdmabuftextureimporter.cpp
    LIBRARIES
        Qt::MultimediaPrivate
        Qt::MultimediaTestLibPrivate
        Qt::GuiPrivate
)
