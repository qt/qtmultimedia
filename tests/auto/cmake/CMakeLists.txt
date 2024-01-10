# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(qtmultimedia_cmake_tests)

enable_testing()

find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 OPTIONAL_COMPONENTS Widgets)

include("${_Qt6CTestMacros}")

set(qt_module_includes
    Multimedia QCamera
)

if(TARGET Qt6::Widgets)
    list(APPEND qt_module_includes
        MultimediaWidgets QVideoWidget
    )
endif()

_qt_internal_test_module_includes(
    ${qt_module_includes}
)
