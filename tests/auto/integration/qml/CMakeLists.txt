# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_policy(SET QTP0001 NEW)

qt_internal_multimedia_add_test(tst_multimedia_integration_qml
    NEEDS_MEDIA_BACKEND_PLUGINS
    QMLTEST
    SOURCES
        tst_multimedia_integration_qml.cpp
        mediaplayer/tst_mediaplayerbackend.qml
        playbackoptions/tst_playbackoptionsbackend.qml
        screencapture/tst_screencapturebackend.qml
        soundeffect/tst_soundeffectbackend.qml
        windowcapture/tst_windowcapturebackend.qml
    LIBRARIES
        Qt::MultimediaPrivate
        Qt::MultimediaTestLibPrivate
        Qt::Qml
        Qt::Quick
        Qt::QuickTest
)

if(TARGET quickmultimediaplugin)
    add_dependencies(tst_multimedia_integration_qml quickmultimediaplugin)
endif()
