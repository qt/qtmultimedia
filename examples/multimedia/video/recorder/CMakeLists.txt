# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(recorder LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/video/recorder")

find_package(Qt6 REQUIRED COMPONENTS Core Multimedia Quick)

qt_add_executable(recorder
    main.cpp
)

set(resource_files
    "main.qml"
    "main_no_permissions.qml"
    "qmldir"
    "MediaList.qml"
    "AudioInputSelect.qml"
    "VideoSourceSelect.qml"
    "RecordButton.qml"
    "Controls.qml"
    "StyleParameter.qml"
    "StyleRectangle.qml"
    "SettingsMetaData.qml"
    "SettingsEncoder.qml"
    "StyleSlider.qml"
    "Style.qml"
    "Playback.qml"
)

qt_add_resources(recorder "recorder"
    PREFIX
        "/"
    FILES
        ${resource_files}
)

target_link_libraries(recorder PRIVATE
    Qt6::Core
    Qt6::Multimedia
    Qt6::Quick
)

set_target_properties(recorder PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
)

install(TARGETS recorder
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
