# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_multimediaquick_qml LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# QML unit tests for the QtMultimedia QML types, run against the mock media
# backend.

# QML files need also to be embedded into the test as TESTDATA
# for Android/iOS tests, since default QML QtTest will
# scan the root qrc for .qml files.
set(QML_SOURCE_FILES
    tst_capturesession.qml
    tst_screencapture.qml
    tst_windowcapture.qml)

qt_internal_multimedia_add_test(tst_multimediaquick_qml
    QMLTEST
    SOURCES
        tst_multimediaquick_qml_screencapturehelper.cpp
        tst_multimediaquick_qml_screencapturehelper.h
        tst_multimediaquick_qml.cpp
        ${QML_SOURCE_FILES}
    TESTDATA
        ${QML_SOURCE_FILES}
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MultimediaQuickPrivate
        Qt::MockMultimediaPlugin
        Qt::Qml
        Qt::Quick
        Qt::QuickPrivate
        Qt::QuickTest
)

if(TARGET quickmultimediaplugin)
    add_dependencies(tst_multimediaquick_qml quickmultimediaplugin)
endif()
