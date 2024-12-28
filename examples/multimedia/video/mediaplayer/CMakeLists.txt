# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(mediaplayer LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/video/mediaplayer")

find_package(Qt6 REQUIRED COMPONENTS Multimedia Core Quick QuickControls2 Svg)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(mediaplayerexample
    main.cpp
)

qt_add_ios_ffmpeg_libraries(mediaplayerexample)

set_target_properties(mediaplayerexample PROPERTIES
    MACOSX_BUNDLE TRUE
)

qt_add_qml_module(mediaplayerexample
    URI mediaplayer
    QML_FILES
        "Main.qml"
        "controls/PlaybackControl.qml"
        "controls/AudioControl.qml"
        "controls/PlaybackSeekControl.qml"
        "controls/SettingsPopup.qml"
        "controls/UrlPopup.qml"
        "controls/MetadataInfo.qml"
        "controls/TracksInfo.qml"
    RESOURCES
        "images/backward10.svg"
        "images/mute.svg"
        "images/open_new.svg"
        "images/pause_symbol.svg"
        "images/play_symbol.svg"
        "images/forward10.svg"
        "images/more.svg"
        "images/speaker.svg"
        "images/stop_symbol.svg"
        "images/volume.svg"
        "images/volume_mute.svg"
        "images/zoom_maximize.svg"
        "images/zoom_minimize.svg"
        "images/link.svg"
        "images/loop.svg"
)

target_link_libraries(mediaplayerexample PRIVATE
    Qt6::Core
    Qt6::Multimedia
    Qt6::Svg
    Qt6::Quick
    Qt6::Multimedia
)

install(TARGETS mediaplayerexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
