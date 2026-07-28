# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_policy(SET QTP0001 NEW)

qt_add_library(tst_multimedia_qml_backendutils STATIC)
qt_autogen_tools_initial_setup(tst_multimedia_qml_backendutils)
qt_add_qml_module(tst_multimedia_qml_backendutils
    URI QtMultimediaTest
    VERSION 1.0
    SOURCES
        mediabackendutils.h
        mediabackendutils.cpp
)
target_link_libraries(tst_multimedia_qml_backendutils PRIVATE Qt::MultimediaTestLibPrivate)

qt_internal_multimedia_add_test(tst_multimedia_qml
    NEEDS_MEDIA_BACKEND_PLUGINS
    QMLTEST
    SOURCES
        tst_qml.cpp
        mediaplayer/tst_mediaplayer.qml
        playbackoptions/tst_playbackoptions.qml
        screencapture/tst_screencapture.qml
        soundeffect/tst_soundeffect.qml
        windowcapture/tst_windowcapture.qml
    LIBRARIES
        Qt::MultimediaPrivate
        Qt::MultimediaTestLibPrivate
        Qt::Quick
        Qt::QuickTest
)

qt_autogen_tools_initial_setup(tst_multimedia_qml_backendutilsplugin)
target_link_libraries(tst_multimedia_qml PRIVATE tst_multimedia_qml_backendutilsplugin)

if(TARGET quickmultimediaplugin)
    add_dependencies(tst_multimedia_qml quickmultimediaplugin)
endif()
