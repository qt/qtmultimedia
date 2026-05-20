# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(gstreamer-video-source-rtp LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/gstreamer-video-source-rtp")

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia MultimediaWidgets GstreamerMediaPluginImplPrivate)
endif()

qt_add_executable( gstreamer-video-source-rtp WIN32 MACOSX_BUNDLE
    gstreamer-video-source-rtp.cpp
    Info.plist.in
)

target_link_libraries(gstreamer-video-source-rtp PUBLIC
    Qt::Widgets
    Qt::Multimedia
    Qt::MultimediaPrivate
    Qt::MultimediaWidgets

    Qt::GstreamerMediaPluginImplPrivate
)

install(TARGETS gstreamer-video-source-rtp
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

set_target_properties(gstreamer-video-source-rtp PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
)
