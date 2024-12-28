# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(qmlvideoexample LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/video/qmlvideo")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Multimedia Qml Quick)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(qmlvideoexample
    main.cpp
    trace.h
)

qt_add_ios_ffmpeg_libraries(qmlvideoexample)

set_target_properties(qmlvideoexample PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
    MACOSX_BUNDLE_GUI_IDENTIFIER "io.qt.examples.qmlvideo"
)

add_subdirectory(qmlvideo)
add_subdirectory(frequencymonitor)

target_link_libraries(qmlvideoexample PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Multimedia
    Qt::Qml
    Qt::Quick
    qmlvideo
    frequencymonitor
)

install(TARGETS qmlvideoexample
    BUNDLE DESTINATION .
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

qt_generate_deploy_qml_app_script(
    TARGET qmlvideoexample
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)

install(SCRIPT ${deploy_script})
