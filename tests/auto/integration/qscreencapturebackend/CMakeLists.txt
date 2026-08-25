# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qscreencapturebackend LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_multimedia_add_test(tst_qscreencapturebackend
    NEEDS_MEDIA_BACKEND_PLUGINS
    SOURCES
        tst_qscreencapturebackend.cpp
    LIBRARIES
        Qt::Multimedia
        Qt::MultimediaPrivate
        Qt::Gui
        Qt::Widgets
        Qt::MultimediaTestLibPrivate
)

if(QT_FEATURE_pipewire_screencapture)
    target_compile_definitions(tst_qscreencapturebackend
        PRIVATE QT_MM_HAVE_FAKE_XDG_PORTAL)
endif()

if(ANDROID)
    set_property(TARGET tst_qscreencapturebackend
        PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
endif()
