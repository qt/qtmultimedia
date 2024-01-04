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

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(qmlvideoexample
    frequencymonitor.cpp frequencymonitor.h
    frequencymonitordeclarative.cpp
    performancemonitor.cpp performancemonitor.h
    performancemonitordeclarative.cpp performancemonitordeclarative.h
    main.cpp
    trace.h
)

set_target_properties(qmlvideoexample PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
)

target_compile_definitions(qmlvideoexample PUBLIC
    FREQUENCYMONITOR_SUPPORT
    PERFORMANCEMONITOR_SUPPORT
)

add_subdirectory(qmlvideo)
add_subdirectory(performancemonitor)
add_subdirectory(frequencymonitor)

target_link_libraries(qmlvideoexample PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Multimedia
    Qt::Qml
    Qt::Quick
    qmlvideo
    performancemonitor
    frequencymonitor
)

install(TARGETS qmlvideoexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
