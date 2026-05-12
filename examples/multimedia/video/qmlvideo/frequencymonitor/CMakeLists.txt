# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(FATAL_ERROR "This module is part of the 'qmlvideo' example, and should not be built independently.")
endif()

qt_add_qml_module(frequencymonitor
    URI frequencymonitor
    STATIC
    VERSION 1.0
    QML_FILES
        "FrequencyItem.qml"
        "PerformanceItem.qml"
    SOURCES
        frequencymonitor.h frequencymonitor.cpp
)

target_link_libraries(frequencymonitor PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
)
