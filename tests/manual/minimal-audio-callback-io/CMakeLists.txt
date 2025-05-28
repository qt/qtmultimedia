# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(minimal-audio-callback-io LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/minimal-audio-callback-io")

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    find_package(Qt6 REQUIRED COMPONENTS Multimedia MultimediaPrivate)
endif()

qt_add_executable( minimal-audio-callback-io MACOSX_BUNDLE
    minimal-audio-callback-io.cpp
    ../../../src/3rdparty/dr_libs/dr_wav.h
)

target_include_directories( minimal-audio-callback-io SYSTEM PRIVATE
    ../../../src/3rdparty/dr_libs
)

target_link_libraries( minimal-audio-callback-io PUBLIC
    Qt::Multimedia
    Qt::MultimediaPrivate
)

install(TARGETS minimal-audio-callback-io
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

set_target_properties( minimal-audio-callback-io PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
)
