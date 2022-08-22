# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_plugin(QWindowsMediaPlugin
    OUTPUT_NAME windowsmediaplugin
    PLUGIN_TYPE multimedia
    SOURCES
        common/mfmetadata.cpp common/mfmetadata_p.h
        decoder/mfaudiodecodercontrol.cpp decoder/mfaudiodecodercontrol_p.h
        decoder/mfdecodersourcereader.cpp decoder/mfdecodersourcereader_p.h
        evr/evrcustompresenter.cpp evr/evrcustompresenter_p.h
        evr/evrd3dpresentengine.cpp evr/evrd3dpresentengine_p.h
        evr/evrhelpers.cpp evr/evrhelpers_p.h
        evr/evrvideowindowcontrol.cpp evr/evrvideowindowcontrol_p.h
        mfstream.cpp mfstream_p.h
        player/mfactivate.cpp player/mfactivate_p.h
        player/mfevrvideowindowcontrol.cpp player/mfevrvideowindowcontrol_p.h
        player/mfplayercontrol.cpp player/mfplayercontrol_p.h
        player/mfplayersession.cpp player/mfplayersession_p.h
        player/mftvideo.cpp player/mftvideo_p.h
        player/mfvideorenderercontrol.cpp player/mfvideorenderercontrol_p.h
        player/samplegrabber.cpp player/samplegrabber_p.h
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
        qwindowsvideodevices.cpp qwindowsvideodevices_p.h
        sourceresolver.cpp sourceresolver_p.h
    INCLUDE_DIRECTORIES
        audio
        common
        decoder
        evr
        player
        mediacapture
    LIBRARIES
        Qt::MultimediaPrivate
        Qt::CorePrivate
        uuid
        WMF::WMF
        d3d9
        dxva2
        evr
        gdi32
        ksuser
        mf
        mfcore
        mfplat
        mfreadwrite
        mfuuid
        ole32
        oleaut32
        propsys
        shlwapi
        strmiids
        amstrmid
        user32
        winmm
        wmcodecdspuuid
)

