# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qwindowcapturebackend LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_multimedia_add_test(tst_qwindowcapturebackend
    NEEDS_MEDIA_BACKEND_PLUGINS
    SOURCES
        tst_qwindowcapturebackend.cpp
        tst_qwindowcapturebackend_widget.h
        tst_qwindowcapturebackend_widget.cpp
        tst_qwindowcapturebackend_fixture.h
        tst_qwindowcapturebackend_fixture.cpp

    LIBRARIES
        Qt::Multimedia
        Qt::MultimediaPrivate
        Qt::Gui
        Qt::Widgets
        Qt::MultimediaWidgets
        Qt::MultimediaTestLibPrivate
)


