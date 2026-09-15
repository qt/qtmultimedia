# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(qml-multiple-players LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/qml-multiple-players")

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Multimedia)
endif()

qt_add_executable(qml-multiple-players MACOSX_BUNDLE
    qml-multiple-players.cpp
)

qt_add_qml_module(qml-multiple-players
    URI QmlMultiplePlayers
    NO_RESOURCE_TARGET_PATH
    QML_FILES
        "qml-multiple-players.qml"
)

target_link_libraries(qml-multiple-players PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
)

set(qml_multiple_players_files
    "testvideo1.mp4"
    "testvideo2.mp4"
)

qt_add_resources( qml-multiple-players "qml-multiple-players"
    PREFIX
        "/"
    FILES
        ${qml_multiple_players_files}
)

foreach(plugin_aggregate IN ITEMS
        qtmultimedia_backend_plugins qtmultimedia_quick_plugins qpa_default_plugins)
    if(TARGET "${plugin_aggregate}")
        add_dependencies(qml-multiple-players "${plugin_aggregate}")
    endif()
endforeach()

install(TARGETS qml-multiple-players
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
