# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

qt_add_library(multimedia_snippets OBJECT
    multimedia-snippets/screencapturesnippets.cpp
    multimedia-snippets/windowcapturesnippets.cpp
)

target_link_libraries(multimedia_snippets PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Multimedia
    Qt::MultimediaWidgets
)

set_target_properties(multimedia_snippets PROPERTIES COMPILE_OPTIONS "-w")

if ("${CMAKE_CXX_COMPILE_FEATURES}" MATCHES "cxx_std_23")
    set_property(TARGET multimedia_snippets PROPERTY CXX_STANDARD 23)
endif()

set_target_properties(multimedia_snippets PROPERTIES UNITY_BUILD OFF)

if(QT_FEATURE_multimedia_quick)
    qt_policy(SET QTP0001 NEW)
    qt_policy(SET QTP0004 NEW)
    qt_add_library(multimedia_qml_snippets STATIC)
    qt_add_qml_module(multimedia_qml_snippets
        URI MultimediaSnippets
        VERSION 1.0
        OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/MultimediaSnippets
        QML_FILES
            multimedia-snippets/screencapturesnippets.qml
            multimedia-snippets/windowcapturesnippets.qml
        DEPENDENCIES
            QtQuick
            QtMultimedia
        NO_PLUGIN
        NO_GENERATE_PLUGIN_SOURCE
        NO_GENERATE_QMLTYPES
    )
endif()
