# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(mediaframeinputs LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/mediaframeinputs")

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia MultimediaWidgets)
endif()

qt_add_executable(mediaframeinputs MACOSX_BUNDLE
    main.cpp
    previewrunner.cpp previewrunner.h
    recordingrunner.cpp recordingrunner.h
    mediagenerator.cpp mediagenerator.h
    pushmodemediasource.h
    mediaframeinputqueue.h
    settings.h
    commandlineparser.cpp commandlineparser.h
)

target_link_libraries(mediaframeinputs PUBLIC
    Qt::Widgets
    Qt::Multimedia
    Qt::MultimediaWidgets
)

install(TARGETS mediaframeinputs
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

if (CLANG OR APPLECLANG OR GCC)
    target_compile_options(mediaframeinputs PRIVATE
        -Wno-c++20-extensions
    )
endif()
