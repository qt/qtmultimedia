# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(CustomMediaInputSnippets LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)

set(PROJECT_SOURCES
    main.cpp
    custommediainputsnippets.cpp
    custommediainputsnippets.h
)

qt_add_executable(CustomMediaInputSnippets
    ${PROJECT_SOURCES}
)

target_link_libraries(CustomMediaInputSnippets PRIVATE
    Qt::Multimedia
)

include(GNUInstallDirs)
install(TARGETS CustomMediaInputSnippets
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
