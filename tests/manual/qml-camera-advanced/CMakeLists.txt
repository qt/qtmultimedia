# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(qml-camera-advanced LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/qml-camera-advanced")

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Multimedia Qml Quick)
endif()
qt_standard_project_setup(REQUIRES 6.9)

qt_add_executable(qml-camera-advanced WIN32 MACOSX_BUNDLE
    main.cpp
)

set_target_properties(qml-camera-advanced PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
)

qt_add_qml_module(qml-camera-advanced
    URI QmlCameraAdvanced
    VERSION 1.0
    QML_FILES
        Main.qml
        CameraFeed.qml
        QuickControls.qml
        AllCameraControls.qml
        InfoPage.qml
        AdvancedPanel.qml
        CameraDeviceSelector.qml
        CameraHelper.qml
        ImageCaptureHelper.qml
        MediaDevicesHelper.qml
)

target_link_libraries(qml-camera-advanced PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Multimedia
    Qt::Qml
    Qt::Quick
)

qt_add_ios_ffmpeg_libraries(qml-camera-advanced)

install(TARGETS qml-camera-advanced
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
