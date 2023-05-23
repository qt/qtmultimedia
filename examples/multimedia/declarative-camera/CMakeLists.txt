# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(declarative-camera LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/declarative-camera")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Multimedia Qml Quick)

qt_add_executable(declarative-camera
    qmlcamera.cpp
)

set_target_properties(declarative-camera PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
)

target_include_directories(declarative-camera PUBLIC
    ../shared
)

target_link_libraries(declarative-camera PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Multimedia
    Qt::Qml
    Qt::Quick
)

# Resources:
set(declarative-camera_resource_files
    "CameraButton.qml"
    "CameraListButton.qml"
    "CameraListPopup.qml"
    "CameraPropertyButton.qml"
    "CameraPropertyPopup.qml"
    "FlashControl.qml"
    "PhotoCaptureControls.qml"
    "PhotoPreview.qml"
    "Popup.qml"
    "VideoCaptureControls.qml"
    "VideoPreview.qml"
    "ZoomControl.qml"
    "declarative-camera.qml"
    "images/camera_auto_mode.png"
    "images/camera_camera_setting.png"
    "images/camera_flash_auto.png"
    "images/camera_flash_fill.png"
    "images/camera_flash_off.png"
    "images/camera_flash_redeye.png"
    "images/camera_white_balance_cloudy.png"
    "images/camera_white_balance_flourescent.png"
    "images/camera_white_balance_incandescent.png"
    "images/camera_white_balance_sunny.png"
    "images/toolbutton.png"
    "images/toolbutton.sci"
)

qt_add_resources(declarative-camera "declarative-camera"
    PREFIX
        "/"
    FILES
        ${declarative-camera_resource_files}
)

install(TARGETS declarative-camera
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
