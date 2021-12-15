# Generated from declarative-camera.pro.

cmake_minimum_required(VERSION 3.16)
project(declarative-camera LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/declarative-camera")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Multimedia)

qt_add_executable(declarative-camera
    qmlcamera.cpp
)
set_target_properties(declarative-camera PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
    XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS
        "${CMAKE_CURRENT_LIST_DIR}/declarative-camera.entitlements"
)
# special case begin
target_include_directories(declarative-camera PUBLIC
    ../shared
)
# special case end
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

qt6_add_resources(declarative-camera "declarative-camera"
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

if(APPLE AND NOT IOS)
    if(NOT CMAKE_GENERATOR STREQUAL "Xcode")
        add_custom_command(TARGET declarative-camera
        POST_BUILD
        COMMAND codesign --sign - --entitlements ${CMAKE_CURRENT_SOURCE_DIR}/declarative-camera.entitlements ${CMAKE_CURRENT_BINARY_DIR}/declarative-camera.app
    )
    endif()
endif()
