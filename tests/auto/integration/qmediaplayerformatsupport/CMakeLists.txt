# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qmediaplayerformatsupport LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_multimedia_add_test(tst_qmediaplayerformatsupport
    NEEDS_MEDIA_BACKEND_PLUGINS
    SOURCES
        tst_qmediaplayerformatsupport.cpp
    LIBRARIES
        Qt::Core
        Qt::MultimediaPrivate
        Qt::MultimediaTestLibPrivate
    BUILTIN_TESTDATA
    TESTDATA
        testdata/README.md
        testdata/containers/supported/container.avi
        testdata/containers/supported/container.mkv
        testdata/containers/supported/container.mp4
        testdata/containers/supported/container.mpeg
        testdata/containers/supported/container.wmv
        testdata/containers/supported/container.webp
        testdata/flipable.gif
        testdata/pixel_formats/supported/h264_bgr0.mp4
        testdata/pixel_formats/supported/h264_bgr24.mp4
        testdata/pixel_formats/supported/h264_gray.mp4
        testdata/pixel_formats/supported/h264_gray10le.mp4
        testdata/pixel_formats/supported/h264_nv12.mp4
        testdata/pixel_formats/supported/h264_nv16.mp4
        testdata/pixel_formats/supported/h264_nv21.mp4
        testdata/pixel_formats/supported/h264_rgb24.mp4
        testdata/pixel_formats/supported/h264_yuv420p.mp4
        testdata/pixel_formats/supported/h264_yuv420p10.mp4
        testdata/pixel_formats/supported/h264_yuv420p10le.mp4
        testdata/pixel_formats/supported/h264_yuv422p.mp4
        testdata/pixel_formats/supported/h264_yuv422p10.mp4
        testdata/pixel_formats/supported/h264_yuv422p10le.mp4
        testdata/pixel_formats/supported/h264_yuv444p.mp4
        testdata/pixel_formats/supported/h264_yuv444p10.mp4
        testdata/pixel_formats/supported/h264_yuvj420p.mp4
        testdata/pixel_formats/supported/h264_yuvj422p.mp4
        testdata/pixel_formats/supported/h264_yuvj444p.mp4
)
