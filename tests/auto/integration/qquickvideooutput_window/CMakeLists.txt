# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qquickvideooutput_window LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_multimedia_add_test(tst_qquickvideooutput_window
    NEEDS_MEDIA_BACKEND_PLUGINS
    SOURCES
        tst_qquickvideooutput_window.cpp
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MultimediaQuickPrivate
        Qt::MultimediaTestLibPrivate
        Qt::Qml
        Qt::Quick
)

# Resources:
set(qml_resource_files
    "main.qml"
)

qt_internal_add_resource(tst_qquickvideooutput_window "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)

