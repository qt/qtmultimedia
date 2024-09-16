# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(audiodevices LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/audiodevices")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Multimedia Widgets)

qt_add_executable(audiodevices
    audiodevices.cpp audiodevices.h
    audiodevicesbase.ui
    main.cpp
)

set_target_properties(audiodevices PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
)

target_link_libraries(audiodevices PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Multimedia
    Qt::Widgets
)

qt_add_ios_ffmpeg_libraries(audiodevices)

install(TARGETS audiodevices
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
