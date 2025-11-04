# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(recorder LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/video/recorder")

find_package(Qt6 REQUIRED COMPONENTS Core Multimedia Quick)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(recorder
    main.cpp
)

qt_add_ios_ffmpeg_libraries(recorder)

set(qml_singleton_files
    Style.qml
    PermissionHelper.qml)
set_source_files_properties(
    ${qml_singleton_files}
    PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

set(qml_files
    "Main.qml"
    "MediaList.qml"
    "AudioInputSelect.qml"
    "VideoSourceSelect.qml"
    "RecordButton.qml"
    "Controls.qml"
    "StyleParameter.qml"
    "StyleRectangle.qml"
    "SettingsMetaData.qml"
    "SettingsEncoder.qml"
    "Style.qml"
    "PermissionHelper.qml"
    "Playback.qml"
)

qt_add_qml_module(recorder
    URI RecorderExample
    QML_FILES
        ${qml_files}
)

target_link_libraries(recorder PRIVATE
    Qt6::Core
    Qt6::Multimedia
    Qt6::Quick
)

set_target_properties(recorder PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
    QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android
)

install(TARGETS recorder
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
