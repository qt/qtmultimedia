# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_multimedia_static_linking LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_multimedia_add_test(tst_multimedia_static_linking
    NEEDS_MEDIA_BACKEND_PLUGINS
    SOURCES
        tst_multimedia_static_linking.cpp
    LIBRARIES
        Qt::Multimedia
)

foreach(plugin Android Darwin Gstreamer Qnx FFMpeg Wasm Windows)
    if (TARGET Qt::Q${plugin}MediaPlugin)
        target_link_libraries(tst_multimedia_static_linking PUBLIC Qt::Q${plugin}MediaPlugin)
    endif()
endforeach()
