# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


qt_internal_add_plugin(QDarwinMediaPlugin
    OUTPUT_NAME darwinmediaplugin
    PLUGIN_TYPE multimedia
    SOURCES
        avfaudiodecoder.mm avfaudiodecoder_p.h
        mediaplayer/avfdisplaylink.mm mediaplayer/avfdisplaylink_p.h
        mediaplayer/avfmediaplayer.mm mediaplayer/avfmediaplayer_p.h
        common/avfmetadata.mm common/avfmetadata_p.h
        mediaplayer/avfvideorenderercontrol.mm mediaplayer/avfvideorenderercontrol_p.h
        avfvideosink.mm avfvideosink_p.h
        avfvideobuffer.mm avfvideobuffer_p.h
        qdarwinformatsinfo.mm qdarwinformatsinfo_p.h
        qdarwinintegration.mm qdarwinintegration_p.h
    INCLUDE_DIRECTORIES
        audio
        camera
        common
        mediaplayer
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
        QT_NO_CONTEXTLESS_CONNECT
    LIBRARIES
        Qt::MultimediaPrivate
        Qt::CorePrivate
        ${FWAudioToolbox}
        ${FWCoreMedia}
        ${FWCoreVideo}
        ${FWFoundation}
        ${FWMetal}
        ${FWQuartzCore}
        AVFoundation::AVFoundation
)

qt_internal_extend_target(QDarwinMediaPlugin CONDITION NOT TVOS AND NOT VISIONOS
    SOURCES
        camera/qavfcamerabase.mm camera/qavfcamerabase_p.h
        camera/avfcamera.mm camera/avfcamera_p.h
        camera/avfaudiopreviewdelegate.mm camera/avfaudiopreviewdelegate_p.h
        camera/avfcamerarenderer.mm camera/avfcamerarenderer_p.h
        camera/avfcameraservice.mm camera/avfcameraservice_p.h
        camera/avfcamerasession.mm camera/avfcamerasession_p.h
        camera/avfimagecapture.mm camera/avfimagecapture_p.h
        camera/avfmediaassetwriter.mm camera/avfmediaassetwriter_p.h
        camera/avfmediaencoder.mm camera/avfmediaencoder_p.h
)

qt_internal_extend_target(QDarwinMediaPlugin CONDITION MACOS
    LIBRARIES
        ${FWAppKit}
        ${FWAudioUnit}
        ${FWVideoToolbox}
        ${FWApplicationServices}
)

qt_internal_extend_target(QDarwinMediaPlugin CONDITION IOS OR VISIONOS
    LIBRARIES
        ${FWCoreGraphics}
        ${FWCoreVideo}
)
