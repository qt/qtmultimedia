# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qpipewirecamera LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

if(NOT QT_FEATURE_pipewire OR NOT QT_FEATURE_dbus)
    return()
endif()

qt_find_package(PipeWire PROVIDED_TARGETS PipeWire::PipeWire MODULE_NAME multimedia QMAKE_LIB pipewire)

qt_internal_multimedia_add_test(tst_qpipewirecamera
    NEEDS_MEDIA_BACKEND_PLUGINS
    SOURCES
        tst_qpipewirecamera.cpp
    LIBRARIES
        Qt::Gui
        Qt::Multimedia
        Qt::MultimediaPrivate
        Qt::MultimediaTestLibPrivate
        PipeWire::PipeWire
)

set_property(TEST tst_qpipewirecamera APPEND PROPERTY LABELS pipewire_camera)
