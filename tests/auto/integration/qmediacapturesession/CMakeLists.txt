# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qmediacapturesession LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_multimedia_add_test(tst_qmediacapturesession
    NEEDS_MEDIA_BACKEND_PLUGINS
    MACOS_BUNDLE
    SOURCES
        tst_qmediacapturesession.cpp
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MultimediaWidgets
        Qt::MultimediaTestLibPrivate
)

if(OHOS)
    # The stress_test_setup_and_teardown_* subtests can run for tens of seconds
    # without emitting any progress, which trips the 60 s no-progress watchdog
    # in harmonyostestrunner. Disable that watchdog for this test; the overall
    # test timeout still bounds runtime.
    set_property(TEST tst_qmediacapturesession APPEND PROPERTY
        ENVIRONMENT "HARMONYOS_NO_PROGRESS_TIMEOUT=0")
endif()
