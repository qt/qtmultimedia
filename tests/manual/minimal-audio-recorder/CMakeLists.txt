# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(sidepanel LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/minimal-audio-recorder")

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia MultimediaWidgets)
endif()

qt_add_executable( minimal-audio-recorder MACOSX_BUNDLE
    minimal-audio-recorder.cpp
)

target_link_libraries( minimal-audio-recorder PUBLIC
    Qt::Widgets
    Qt::Multimedia
    Qt::MultimediaWidgets
)

install(TARGETS  minimal-audio-recorder
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

set_target_properties( minimal-audio-recorder PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
)
