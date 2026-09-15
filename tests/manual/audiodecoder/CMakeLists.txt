# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(audiodecoder LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/audiodecoder")

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Multimedia Widgets)
endif()

if(ANDROID)

endif()

qt_add_executable(audiodecoder
    audiodecoder.cpp audiodecoder.h
    main.cpp
)

set_target_properties(audiodecoder PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(audiodecoder PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Multimedia
)

if(ANDROID)
    target_link_libraries(audiodecoder PUBLIC Qt::Widgets)
endif()

foreach(plugin_aggregate IN ITEMS qtmultimedia_backend_plugins qpa_default_plugins)
    if(TARGET "${plugin_aggregate}")
        add_dependencies(audiodecoder "${plugin_aggregate}")
    endif()
endforeach()

install(TARGETS audiodecoder
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
