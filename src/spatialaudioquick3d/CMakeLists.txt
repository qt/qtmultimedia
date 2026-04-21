# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## Quick3D.Sound Module:
#####################################################################

qt_internal_add_qml_module(Quick3DSpatialAudio
    URI "QtQuick3D.SpatialAudio"
    VERSION "${PROJECT_VERSION}"
    CLASS_NAME QQuick3DAudioModule
    PLUGIN_TARGET quick3dspatialaudioplugin
    NO_GENERATE_PLUGIN_SOURCE
    NO_PLUGIN_OPTIONAL
    DEPENDENCIES QtQuick QtMultimedia
    SOURCES
        qquick3daudiolistener.cpp qquick3daudiolistener_p.h
        qquick3daudioroom.cpp qquick3daudioroom_p.h
        qquick3dspatialsound.cpp qquick3dspatialsound_p.h
        qquick3dambientsound.cpp qquick3dambientsound_p.h
        qquick3daudioengine.cpp qquick3daudioengine_p.h
        qtquick3daudioglobal.h qtquick3daudioglobal_p.h
        qtquick3daudiotypes_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    QML_FILES
    LIBRARIES
        Qt::Quick3DPrivate
        Qt::SpatialAudioPrivate
        Qt::MultimediaQuickPrivate
    PUBLIC_LIBRARIES
        Qt::SpatialAudio
    PRIVATE_MODULE_INTERFACE
        Qt::Quick3DPrivate
        Qt::SpatialAudioPrivate
        Qt::MultimediaQuickPrivate
)

qt_internal_extend_target(quick3dspatialaudioplugin
    SOURCES
        qquick3dspatialaudio_plugin.cpp
)
