# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_gstreamer_backend LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_multimedia_add_test(tst_gstreamer_backend
    SOURCES
        tst_gstreamer_backend.cpp
        tst_gstreamer_backend.h
    LIBRARIES
        Qt::MultimediaPrivate
        Qt::GstreamerMediaPluginImplPrivate

    BUILTIN_TESTDATA
    TESTDATA
        "metadata_test_file.mp4"
        "color_matrix_90_deg_clockwise.mp4"
        "color_matrix_90_deg_clockwise.m2v"
)
