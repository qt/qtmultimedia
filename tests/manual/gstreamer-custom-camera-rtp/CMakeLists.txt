# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(gstreamer-custom-camera-rtp LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/gstreamer-custom-camera-rtp")

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia MultimediaWidgets GstreamerMediaPluginImplPrivate)
endif()

qt_add_executable( gstreamer-custom-camera-rtp WIN32 MACOSX_BUNDLE
    gstreamer-custom-camera-rtp.cpp
    Info.plist.in
)

target_link_libraries( gstreamer-custom-camera-rtp PUBLIC
    Qt::Widgets
    Qt::Multimedia
    Qt::MultimediaPrivate
    Qt::MultimediaWidgets

    Qt::GstreamerMediaPluginImplPrivate
)

install(TARGETS gstreamer-custom-camera-rtp
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

set_target_properties( gstreamer-custom-camera-rtp PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
)
