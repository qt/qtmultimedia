# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_find_package(EGL)
qt_find_package(VAAPI COMPONENTS VA DRM PROVIDED_TARGETS VAAPI::VA VAAPI::DRM MODULE_NAME multimedia QMAKE_LIB vaapi)

qt_internal_find_apple_system_framework(FWCoreMedia CoreMedia) # special case
qt_internal_find_apple_system_framework(FWCoreAudio CoreAudio) # special case
qt_internal_find_apple_system_framework(FWAudioUnit AudioUnit) # special case
qt_internal_find_apple_system_framework(FWVideoToolbox VideoToolbox) # special case
qt_internal_find_apple_system_framework(FWAVFoundation AVFoundation) # special case

qt_internal_add_plugin(QFFmpegMediaPlugin
    OUTPUT_NAME ffmpegmediaplugin
    PLUGIN_TYPE multimedia
    SOURCES
        qffmpeg_p.h
        qffmpegaudiodecoder.cpp qffmpegaudiodecoder_p.h
        qffmpegaudioinput.cpp qffmpegaudioinput_p.h
        qffmpegclock.cpp qffmpegclock_p.h
        qffmpeghwaccel.cpp qffmpeghwaccel_p.h
        qffmpegencoderoptions.cpp qffmpegencoderoptions_p.h
        qffmpegmediametadata.cpp qffmpegmediametadata_p.h
        qffmpegmediaplayer.cpp qffmpegmediaplayer_p.h
        qffmpegvideosink.cpp qffmpegvideosink_p.h
        qffmpegmediaformatinfo.cpp qffmpegmediaformatinfo_p.h
        qffmpegmediaintegration.cpp qffmpegmediaintegration_p.h
        qffmpegvideobuffer.cpp qffmpegvideobuffer_p.h
        qffmpegimagecapture.cpp qffmpegimagecapture_p.h
        qffmpegmediacapturesession.cpp qffmpegmediacapturesession_p.h
        qffmpegmediarecorder.cpp qffmpegmediarecorder_p.h
        qffmpegencoder.cpp qffmpegencoder_p.h
        qffmpegthread.cpp qffmpegthread_p.h
        qffmpegresampler.cpp qffmpegresampler_p.h
        qffmpegvideoframeencoder.cpp qffmpegvideoframeencoder_p.h
        qffmpegscreencapturebase.cpp qffmpegscreencapturebase_p.h
        qffmpegscreencapture.cpp qffmpegscreencapture_p.h

        qffmpegplaybackengine.cpp qffmpegplaybackengine_p.h
        playbackengine/qffmpegplaybackengineobject.cpp playbackengine/qffmpegplaybackengineobject_p.h
        playbackengine/qffmpegdemuxer.cpp playbackengine/qffmpegdemuxer_p.h
        playbackengine/qffmpegstreamdecoder.cpp playbackengine/qffmpegstreamdecoder_p.h
        playbackengine/qffmpegrenderer.cpp playbackengine/qffmpegrenderer_p.h
        playbackengine/qffmpegaudiorenderer.cpp playbackengine/qffmpegaudiorenderer_p.h
        playbackengine/qffmpegvideorenderer.cpp playbackengine/qffmpegvideorenderer_p.h
        playbackengine/qffmpegsubtitlerenderer.cpp playbackengine/qffmpegsubtitlerenderer_p.h
        playbackengine/qffmpegtimecontroller.cpp playbackengine/qffmpegtimecontroller_p.h
        playbackengine/qffmpegmediadataholder.cpp playbackengine/qffmpegmediadataholder_p.h
        playbackengine/qffmpegcodec.cpp playbackengine/qffmpegcodec_p.h
        playbackengine/qffmpegpacket_p.h
        playbackengine/qffmpegframe_p.h
    DEFINES
        QT_COMPILING_FFMPEG
    LIBRARIES
        Qt::MultimediaPrivate
        Qt::CorePrivate
        FFmpeg::avformat FFmpeg::avcodec FFmpeg::swresample FFmpeg::swscale FFmpeg::avutil
)

qt_internal_extend_target(QFFmpegMediaPlugin CONDITION QT_FEATURE_ffmpeg AND QT_FEATURE_vaapi
    SOURCES
        qffmpeghwaccel_vaapi.cpp qffmpeghwaccel_vaapi_p.h
    LIBRARIES
        VAAPI::VAAPI
        EGL::EGL
)


qt_internal_extend_target(QFFmpegMediaPlugin CONDITION APPLE
    SOURCES
        ../darwin/qavfhelpers.mm ../darwin/qavfhelpers_p.h
        ../darwin/camera/qavfcamerabase_p.h ../darwin/camera/qavfcamerabase.mm
        ../darwin/camera/avfcamerautility_p.h ../darwin/camera/avfcamerautility.mm
        qffmpeghwaccel_videotoolbox.mm qffmpeghwaccel_videotoolbox_p.h
        qavfcamera.mm qavfcamera_p.h
        qavfsamplebufferdelegate.mm qavfsamplebufferdelegate_p.h
        qavfscreencapture.mm qavfscreencapture_p.h
    INCLUDE_DIRECTORIES
        ../darwin
        ../darwin/camera
    LIBRARIES
        ${FWAudioToolbox}
        ${FWCoreAudio}
        ${FWCoreFoundation}
        ${FWCoreMedia}
        ${FWCoreVideo}
        ${FWVideoToolbox}
        AVFoundation::AVFoundation
)

qt_internal_extend_target(QFFmpegMediaPlugin CONDITION WIN32
    SOURCES
        ../windows/qwindowsvideodevices.cpp ../windows/qwindowsvideodevices_p.h
        qwindowscamera.cpp qwindowscamera_p.h
        qffmpeghwaccel_d3d11.cpp qffmpeghwaccel_d3d11_p.h
        qffmpegscreencapture_dxgi.cpp qffmpegscreencapture_dxgi_p.h
    INCLUDE_DIRECTORIES
        ../windows
    LIBRARIES
        Qt::MultimediaPrivate
        WMF::WMF
        mfreadwrite
)

qt_internal_extend_target(QFFmpegMediaPlugin CONDITION QT_FEATURE_cpp_winrt
    SOURCES
        qffmpegscreencapture_uwp.cpp qffmpegscreencapture_uwp_p.h
    INCLUDE_DIRECTORIES
        ../windows
    LIBRARIES
        Dwmapi
        Dxva2
        windowsapp
)

qt_internal_extend_target(QFFmpegMediaPlugin CONDITION QT_FEATURE_xlib
    SOURCES
        qx11screencapture.cpp qx11screencapture_p.h
    LIBRARIES
        X11
        Xrandr
        Xext
)

set_source_files_properties(qx11screencapture.cpp # X headers
                            PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)

qt_internal_extend_target(QFFmpegMediaPlugin CONDITION QT_FEATURE_linux_v4l
    SOURCES
        qv4l2camera.cpp qv4l2camera_p.h
)

if (ANDROID)
    qt_internal_extend_target(QFFmpegMediaPlugin
        SOURCES
            qffmpeghwaccel_mediacodec.cpp qffmpeghwaccel_mediacodec_p.h
            qandroidcamera_p.h qandroidcamera.cpp
            qandroidvideodevices.cpp qandroidvideodevices_p.h
            ../android/wrappers/jni/androidsurfacetexture_p.h
            ../android/wrappers/jni/androidsurfacetexture.cpp
        INCLUDE_DIRECTORIES
            ${FFMPEG_DIR}/include
            ../android/wrappers/jni/
        LIBRARIES
            OpenSLES
            mediandk
            android
    )

    set_property(TARGET QFFmpegMediaPlugin APPEND PROPERTY QT_ANDROID_LIB_DEPENDENCIES
        ${INSTALL_PLUGINSDIR}/multimedia/libplugins_multimedia_ffmpegmediaplugin.so
    )

    set_property(TARGET QFFmpegMediaPlugin APPEND PROPERTY QT_ANDROID_PERMISSIONS
        android.permission.CAMERA
        android.permission.RECORD_AUDIO
        android.permission.BLUETOOTH
        android.permission.MODIFY_AUDIO_SETTINGS
    )
endif()
