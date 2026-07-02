# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(quick3d-spatialaudio LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/quick3d-spatialaudio")

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Multimedia)
endif()

qt_standard_project_setup(REQUIRES 6.12)


qt_add_executable(quick3d-spatialaudio MACOSX_BUNDLE
    main.cpp
)

qt_add_qml_module(quick3d-spatialaudio
    URI Quick3DSpatialAudioTest
    QML_FILES
        Main.qml
    RESOURCES
        nokia-tune.mp3
)

target_link_libraries(quick3d-spatialaudio PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
)


install(TARGETS quick3d-spatialaudio
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
