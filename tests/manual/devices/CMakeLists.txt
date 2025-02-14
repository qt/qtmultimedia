# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(devices LANGUAGES CXX)

if(ANDROID OR IOS)
    message(FATAL_ERROR "This is a commandline tool that is not supported on mobile platforms")
endif()

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/devices")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Multimedia)

qt_add_executable(devices
    main.cpp
)

set_target_properties(devices PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(devices PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Multimedia
)

install(TARGETS devices
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
