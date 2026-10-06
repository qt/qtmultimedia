# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_plugin(QWindowsMediaPlugin
    OUTPUT_NAME windowsmediaplugin
    PLUGIN_TYPE multimedia
    SOURCES
        common/mfmetadata.cpp common/mfmetadata_p.h
        common/qwindowsasfpicture.cpp common/qwindowsasfpicture_p.h
        decoder/mfaudiodecodercontrol.cpp decoder/mfaudiodecodercontrol_p.h
        decoder/mfdecodersourcereader.cpp decoder/mfdecodersourcereader_p.h
        decoder/mfdecodersourcereadercallback.cpp decoder/mfdecodersourcereadercallback_p.h
        mfstream.cpp mfstream_p.h
        player/qmediaengineplayer.cpp player/qmediaengineplayer_p.h
        player/qmediaenginenotify.cpp player/qmediaenginenotify_p.h
        player/qmediaenginetimedtextnotify.cpp player/qmediaenginetimedtextnotify_p.h
        player/qmediaenginevideobuffer.cpp player/qmediaenginevideobuffer_p.h
        qmediaenginevideosink.cpp qmediaenginevideosink_p.h
        qwindowsmfdefs_p.h
        mediacapture/qwindowscamera.cpp
        mediacapture/qwindowscamera_p.h
        mediacapture/qwindowsimagecapture.cpp
        mediacapture/qwindowsimagecapture_p.h
        mediacapture/qwindowsmediacapture.cpp
        mediacapture/qwindowsmediacapture_p.h
        mediacapture/qwindowsmediadevicereader.cpp
        mediacapture/qwindowsmediadevicereader_p.h
        mediacapture/qwindowsmediadevicesession.cpp
        mediacapture/qwindowsmediadevicesession_p.h
        mediacapture/qwindowsmediaencoder.cpp
        mediacapture/qwindowsmediaencoder_p.h
        qwindowsformatinfo.cpp qwindowsformatinfo_p.h
        qwindowsintegration.cpp qwindowsintegration_p.h
        sourceresolver.cpp sourceresolver_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::MultimediaPrivate
        Qt::CorePrivate
        Qt::Concurrent
        uuid
        d3d11
        d3d12
        dxgi
        dxguid
        mf
        mfplat
        mfreadwrite
        mfuuid
        ole32
        propsys
        shlwapi
        user32
)
