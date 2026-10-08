# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


qt_internal_add_plugin(QQnxMediaPlugin
    OUTPUT_NAME qnxmediaplugin
    PLUGIN_TYPE multimedia
    SOURCES
        camera/qqnxcamera.cpp camera/qqnxcamera_p.h
        camera/qqnxplatformcamera.cpp camera/qqnxplatformcamera_p.h
        camera/qqnxcameraframebuffer.cpp camera/qqnxcameraframebuffer_p.h
        camera/qqnximagecapture.cpp camera/qqnximagecapture_p.h
        common/qqnxaudioinput.cpp common/qqnxaudioinput_p.h
        common/qqnxaudiooutput.cpp common/qqnxaudiooutput_p.h
        common/qqnxmediaeventthread.cpp common/qqnxmediaeventthread_p.h
        common/qqnxwindowgrabber.cpp common/qqnxwindowgrabber_p.h
        capture/qqnxaudiorecorder.cpp capture/qqnxaudiorecorder_p.h
        capture/qqnxmediacapturesession.cpp capture/qqnxmediacapturesession_p.h
        capture/qqnxmediarecorder.cpp capture/qqnxmediarecorder_p.h
        mediaplayer/qqnxmediaplayer.cpp mediaplayer/qqnxmediaplayer_p.h
        mediaplayer/qqnxmediametadata.cpp mediaplayer/qqnxmediametadata_p.h
        mediaplayer/qqnxvideosink.cpp mediaplayer/qqnxvideosink_p.h
        mediaplayer/qqnxmediautil.cpp mediaplayer/qqnxmediautil_p.h
        qqnxformatinfo.cpp qqnxformatinfo_p.h
        qqnxmediaintegration.cpp qqnxmediaintegration_p.h
        qqnxvideodevices.cpp qqnxvideodevices_p.h
    INCLUDE_DIRECTORIES
        audio
        camera
        capture
        common
        mediaplayer
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::MultimediaPrivate
        Qt::CorePrivate
        MMRenderer::MMRenderer
        strm
        camapi
)
