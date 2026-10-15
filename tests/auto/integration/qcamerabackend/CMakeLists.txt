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
        Qt::MultimediaTestLibPrivate
)

if(QT_FEATURE_pipewire AND QT_FEATURE_dbus)
    target_compile_definitions(tst_qcamerabackend
        PRIVATE QT_MM_HAVE_FAKE_XDG_PORTAL)
    set_property(TEST tst_qcamerabackend APPEND PROPERTY LABELS pipewire_camera)
endif()

# The following are workarounds to make local testing work for iOS and macOS.
#
# Permission plugins don't work correctly in in-tree builds.
# We use it when we build standalone tests.
# QTBUG-145822
#
# For iOS, we just need to make sure we use the QPermissions API.
# At the time of writing, in-tree builds of tests don't work for iOS because
# deployment requires codesigning, and codesigning only happens with
# Xcode generator.
if(IOS AND QT_INTERNAL_IS_STANDALONE_TEST)
    target_compile_definitions(tst_qcamerabackend
        PUBLIC QT_TST_QCAMERABACKEND_USE_PERMISSION_WORKAROUND)
elseif(APPLE)
    # For in-tree builds for macOS, we explicitly link the permission plugin
    qt_import_plugins(tst_qcamerabackend
        INCLUDE
            Qt6::QDarwinCameraPermissionPlugin
    )
    target_compile_definitions(tst_qcamerabackend
        PUBLIC QT_TST_QCAMERABACKEND_USE_PERMISSION_WORKAROUND)
endif()
