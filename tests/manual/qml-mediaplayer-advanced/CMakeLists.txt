# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(qml-mediaplayer-advanced VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/qml-mediaplayer-advanced")

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Multimedia MultimediaPrivate Qml Quick)
endif()

qt_standard_project_setup(REQUIRES 6.10)

if(ANDROID)
    include(FetchContent)
    FetchContent_Declare(
        android_openssl
        DOWNLOAD_EXTRACT_TIMESTAMP true
        URL https://github.com/KDAB/android_openssl/archive/refs/heads/master.zip
    )
    FetchContent_MakeAvailable(android_openssl)
    include(${android_openssl_SOURCE_DIR}/android_openssl.cmake)
endif()

qt_add_executable(qml-mediaplayer-advanced
    WIN32
    MACOSX_BUNDLE
    main.cpp)

qt_policy(SET QTP0001 NEW)
qt_add_qml_module(qml-mediaplayer-advanced
    URI QmlMediaPlayerAdvanced
    SOURCES
        qmlmediaplayeradvanced.cpp
        qmlmediaplayeradvanced.h
    QML_FILES
        Main.qml
        BackendSettings.qml
        PlayerSettings.qml
        RecentSources.qml
        AudioOutputDeviceSelector.qml
        MediaDeviceInfo.qml
        MediaDevicesHelper.qml
        SettingsWindowButton.qml
    RESOURCES
        onlinesamples.json
)

qt_add_ios_ffmpeg_libraries(qml-mediaplayer-advanced)

if(ANDROID)
    add_android_openssl_libraries(qml-mediaplayer-advanced)
endif()

target_link_libraries(qml-mediaplayer-advanced PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Multimedia
    Qt::MultimediaPrivate
    Qt::Qml
    Qt::Quick)

set_target_properties(qml-mediaplayer-advanced PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in)

foreach(plugin_aggregate IN ITEMS
        qtmultimedia_backend_plugins qtmultimedia_quick_plugins qpa_default_plugins)
    if(TARGET "${plugin_aggregate}")
        add_dependencies(qml-mediaplayer-advanced "${plugin_aggregate}")
    endif()
endforeach()

install(TARGETS qml-mediaplayer-advanced
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
