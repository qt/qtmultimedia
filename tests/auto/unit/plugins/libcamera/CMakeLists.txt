# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qlibcameraformats LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

set(libcamera_plugin_dir "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../src/plugins/libcamera")

# The mapping functions are internal to the libcamera plugin, not exported anywhere -- compile
# the plugin's format-support source directly (plus qlibcamera_instance.cpp, which supplies the
# lcLibCamera logging category qlibcamera_format_support.cpp uses) rather than linking the
# plugin itself, so this test needs neither a running libcamera::CameraManager nor hardware.
qt_internal_multimedia_add_test(tst_qlibcameraformats
    SOURCES
        tst_qlibcameraformats.cpp
        "${libcamera_plugin_dir}/qlibcamera_format_support.cpp"
        "${libcamera_plugin_dir}/qlibcamera_instance.cpp"
    INCLUDE_DIRECTORIES
        "${libcamera_plugin_dir}"
    DEFINES
        QT_NO_KEYWORDS
    LIBRARIES
        LibCamera::LibCamera
        Qt::MultimediaPrivate
        Qt::CorePrivate
)
