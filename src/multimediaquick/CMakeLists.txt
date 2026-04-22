# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## MultimediaQuick Module:
#####################################################################

set(qml_files
    "Video.qml"
)



qt_internal_add_qml_module(MultimediaQuick
    URI "QtMultimedia"
    VERSION "${PROJECT_VERSION}"
    PAST_MAJOR_VERSIONS 5
    CLASS_NAME QMultimediaQuickModule
    PLUGIN_TARGET quickmultimediaplugin
    NO_GENERATE_PLUGIN_SOURCE
    NO_PLUGIN_OPTIONAL
    DEPENDENCIES QtQuick
    SOURCES
        qquickimagecapture.cpp qquickimagecapture_p.h
        qquickimagepreviewprovider.cpp qquickimagepreviewprovider_p.h
        qquickmediaplayer.cpp qquickmediaplayer_p.h
        qquickscreencapture.cpp qquickscreencapture_p.h
        qqmlcontext_source_resolver.cpp qqmlcontext_source_resolver_p.h
        qquicksoundeffect.cpp qquicksoundeffect_p.h
        qquickvideooutput.cpp qquickvideooutput_p.h
        qsgvideonode_p.cpp qsgvideonode_p.h
        qsgvideotexture.cpp qsgvideotexture_p.h
        qtmultimediaquickglobal.h qtmultimediaquickglobal_p.h
        qtmultimediaquicktypes.cpp qtmultimediaquicktypes_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    QML_FILES
        ${qml_files}
    LIBRARIES
        Qt::MultimediaPrivate
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Multimedia
        Qt::Quick
    PRIVATE_MODULE_INTERFACE
        Qt::MultimediaPrivate
        Qt::QuickPrivate
)

qt_internal_extend_target(MultimediaQuick CONDITION QT_FEATURE_gstreamer_qt_api
    SOURCES
        qquickgstreamervideosource.cpp qquickgstreamervideosource_p.h
)

qt_internal_extend_target(quickmultimediaplugin
    SOURCES
        multimedia_plugin.cpp
    LIBRARIES
        Qt::MultimediaQuickPrivate
)

