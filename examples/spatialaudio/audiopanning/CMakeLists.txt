# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(audiodecoder LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/spatialaudio/audiopanning")

find_package(Qt6 REQUIRED COMPONENTS Core Gui SpatialAudio Widgets)

qt_add_executable(audiopanning
    main.cpp
)

qt_add_ios_ffmpeg_libraries(audiopanning)

set_target_properties(audiopanning PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(audiopanning PUBLIC
    Qt::Core
    Qt::SpatialAudio
    Qt::Widgets
)

install(TARGETS audiopanning
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
