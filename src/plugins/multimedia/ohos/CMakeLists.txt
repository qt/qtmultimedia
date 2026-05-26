# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_plugin(QOhosMediaPlugin
    OUTPUT_NAME ohosmediaplugin
    PLUGIN_TYPE multimedia
    SOURCES
        qohosintegration.cpp qohosintegration_p.h
        qohosformatsinfo.cpp qohosformatsinfo_p.h
        qohosglobal_p.h
        common/qohosvideosink.cpp common/qohosvideosink_p.h
        common/qohossurfaceimage.cpp common/qohossurfaceimage_p.h
        common/qohosvideooutput.cpp common/qohosvideooutput_p.h
        mediaplayer/qohosmediaplayer.cpp mediaplayer/qohosmediaplayer_p.h
        mediacapture/qohoscamera.cpp mediacapture/qohoscamera_p.h
        mediacapture/qohoscamerasession.cpp mediacapture/qohoscamerasession_p.h
        mediacapture/qohosimagecapture.cpp mediacapture/qohosimagecapture_p.h
        mediacapture/qohosmediacapturesession.cpp mediacapture/qohosmediacapturesession_p.h
        mediacapture/qohosmediarecorder.cpp mediacapture/qohosmediarecorder_p.h
        mediacapture/qohosvideodevices.cpp mediacapture/qohosvideodevices_p.h
    INCLUDE_DIRECTORIES
        .
        common
        mediaplayer
        mediacapture
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::MultimediaPrivate
        Qt::CorePrivate
        Qt::GuiPrivate
        avplayer
        native_media_codecbase
        native_image
        native_buffer
        native_window
        ohcamera
        ohimage
        image_receiver
        image_source
        image_packer
        avrecorder
)
