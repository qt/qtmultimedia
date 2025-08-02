# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## MultimediaQuickPrivate Module:
#####################################################################

set(qml_files
    "Video.qml"
)

qt_internal_add_qml_module(MultimediaQuickPrivate
    URI "QtMultimedia"
    VERSION "${PROJECT_VERSION}"
    PAST_MAJOR_VERSIONS 5
    CLASS_NAME QMultimediaQuickModule
    PLUGIN_TARGET quickmultimedia
    NO_GENERATE_PLUGIN_SOURCE
    NO_PLUGIN_OPTIONAL
    DEPENDENCIES QtQuick
    CONFIG_MODULE_NAME multimediaquick
    INTERNAL_MODULE
    SOURCES
        qquickimagecapture.cpp qquickimagecapture_p.h
        qquickimagepreviewprovider.cpp qquickimagepreviewprovider_p.h
        qquickmediaplayer.cpp qquickmediaplayer_p.h
        qquickscreencapture.cpp qquickscreencapture_p.h
        qquicksoundeffect.cpp qquicksoundeffect_p.h
        qquickvideooutput.cpp qquickvideooutput_p.h
        qsgvideonode_p.cpp qsgvideonode_p.h
        qsgvideotexture.cpp qsgvideotexture_p.h
        qtmultimediaquickglobal_p.h
        qtmultimediaquicktypes.cpp qtmultimediaquicktypes_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    QML_FILES
        ${qml_files}
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::MultimediaPrivate
        Qt::Quick
        Qt::QuickPrivate
)

target_sources(quickmultimedia PRIVATE multimedia_plugin.cpp)
