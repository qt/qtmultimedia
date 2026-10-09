# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qcamerabackend LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_multimedia_add_test(tst_qcamerabackend
    NEEDS_MEDIA_BACKEND_PLUGINS
    MACOS_BUNDLE
    SOURCES
        tst_qcamerabackend.cpp
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::Widgets
        Qt::MultimediaTestLibPrivate
)

if(QT_FEATURE_pipewire AND QT_FEATURE_dbus)
    target_compile_definitions(tst_qcamerabackend
        PRIVATE QT_MM_HAVE_FAKE_XDG_PORTAL)
    set_property(TEST tst_qcamerabackend APPEND PROPERTY LABELS pipewire_camera)
endif()
