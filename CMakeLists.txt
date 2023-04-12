# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qtmultimedia.pro.

cmake_minimum_required(VERSION 3.16)

include(.cmake.conf)
project(QtMultimedia
    VERSION "${QT_REPO_MODULE_VERSION}"
    DESCRIPTION "Qt Multimedia Libraries"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C
)

find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals Core)
find_package(Qt6 ${PROJECT_VERSION} QUIET CONFIG OPTIONAL_COMPONENTS Gui Network Svg Widgets Quick Qml QuickTest QuickControls2 Quick3D)
qt_internal_project_setup()

if(NOT TARGET Qt::Gui)
    message(NOTICE "Skipping the build as the condition \"TARGET Qt::Gui\" is not met.")
    return()
endif()
if(NOT TARGET Qt::Network)
    message(NOTICE "Skipping the build as the condition \"TARGET Qt::Network\" is not met.")
    return()
endif()

qt_build_repo()
