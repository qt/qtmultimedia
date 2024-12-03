# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(qml-minimal-sound-effect LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/qml-minimal-sound-effect")

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Multimedia Qml Quick)
endif()

qt_add_executable( qml-minimal-sound-effect MACOSX_BUNDLE
    qml-minimal-sound-effect.cpp
)

qt_add_qml_module( qml-minimal-sound-effect
    URI QmlMinimalSoundEffect
    NO_RESOURCE_TARGET_PATH
    QML_FILES
        "qml-minimal-sound-effect.qml"
)

target_link_libraries( qml-minimal-sound-effect PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
)

set(qml_minimal_sound_effect_files
    "double-drop.wav"
    "triple-click.wav"
)

qt_add_resources(qml-minimal-sound-effect "qml-minimal-sound-effect"
    PREFIX
        "/"
    FILES
        ${qml_minimal_sound_effect_files}
)

install(TARGETS qml-minimal-sound-effect
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

set_target_properties( qml-minimal-sound-effect PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
)
