# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_plugin(QWasmMediaPlugin
    OUTPUT_NAME wasmmediaplugin
    PLUGIN_TYPE multimedia
    SOURCES
        qwasmmediaintegration.cpp qwasmmediaintegration_p.h
        mediaplayer/qwasmmediaplayer.cpp mediaplayer/qwasmmediaplayer_p.h
        mediaplayer/qwasmvideosink.cpp mediaplayer/qwasmvideosink_p.h
        common/qwasmvideooutput.cpp common/qwasmvideooutput_p.h
        common/qwasmaudiooutput.cpp common/qwasmaudiooutput_p.h
        common/qwasmaudioinput.cpp common/qwasmaudioinput_p.h
        mediacapture/qwasmmediacapturesession.cpp mediacapture/qwasmmediacapturesession_p.h
        mediacapture/qwasmmediarecorder.cpp mediacapture/qwasmmediarecorder_p.h
        mediacapture/qwasmcamera.cpp mediacapture/qwasmcamera_p.h
        mediacapture/qwasmimagecapture.cpp  mediacapture/qwasmimagecapture_p.h
    INCLUDE_DIRECTORIES
        common
        mediaplayer
        mediacapture
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::MultimediaPrivate
        Qt::CorePrivate
        openal
)

target_link_libraries(QWasmMediaPlugin PUBLIC embind)
