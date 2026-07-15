# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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
        Qt6::Quick
        Qt6::QuickTest
)

if(TARGET quickmultimediaplugin)
    add_dependencies(tst_multimedia_qml quickmultimediaplugin)
endif()
