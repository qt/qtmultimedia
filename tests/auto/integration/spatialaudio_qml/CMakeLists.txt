# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_spatialsound_qml LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_multimedia_add_test(tst_spatialsound_qml
    NEEDS_MEDIA_BACKEND_PLUGINS
    QMLTEST
    SOURCES
        tst_qml.cpp
        qml/tst_spatialsound.qml
        qml/tst_ambientsound.qml
        qml/tst_audioengine.qml
        qml/tst_audiolistener.qml
        qml/tst_audioroom.qml
    LIBRARIES
        Qt6::Quick
        Qt6::QuickTest
    TESTDATA
        "qml/test.wav"
)
