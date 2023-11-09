# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_find_package(EGL)
qt_find_package(VAAPI COMPONENTS VA DRM PROVIDED_TARGETS VAAPI::VAAPI MODULE_NAME multimedia QMAKE_LIB vaapi)

qt_internal_find_apple_system_framework(FWCoreMedia CoreMedia)
qt_internal_find_apple_system_framework(FWCoreAudio CoreAudio)
qt_internal_find_apple_system_framework(FWAudioUnit AudioUnit)
qt_internal_find_apple_system_framework(FWVideoToolbox VideoToolbox)
qt_internal_find_apple_system_framework(FWAVFoundation AVFoundation)
qt_internal_find_apple_system_framework(FWSecurity Security)

qt_internal_add_plugin(QFFmpegMediaPlugin
    OUTPUT_NAME ffmpegmediaplugin
    PLUGIN_TYPE multimedia
    SOURCES
        qffmpeg.cpp qffmpeg_p.h
        qffmpegaudiodecoder.cpp qffmpegaudiodecoder_p.h
        qffmpegaudioinput.cpp qffmpegaudioinput_p.h
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
        qffmpegvideoencoderutils.cpp qffmpegvideoencoderutils_p.h
        qgrabwindowsurfacecapture.cpp qgrabwindowsurfacecapture_p.h
        qffmpegsurfacecapturethread.cpp qffmpegsurfacecapturethread_p.h
        qffmpegsymbolsresolve_p.h

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
        playbackengine/qffmpegpositionwithoffset_p.h
    DEFINES
        QT_COMPILING_FFMPEG
    LIBRARIES
        Qt::MultimediaPrivate
        Qt::CorePrivate
        FFmpeg::avformat FFmpeg::avcodec FFmpeg::swresample FFmpeg::swscale FFmpeg::avutil
)

if(DYNAMIC_RESOLVE_OPENSSL_SYMBOLS)
    if(NOT OPENSSL_INCLUDE_DIR AND OPENSSL_ROOT_DIR)
        set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/include")
    endif()
endif()

qt_internal_extend_target(QFFmpegMediaPlugin CONDITION DYNAMIC_RESOLVE_OPENSSL_SYMBOLS
    SOURCES
        qffmpegopensslsymbols.cpp
    INCLUDE_DIRECTORIES
        ${OPENSSL_INCLUDE_DIR}
)

if (ENABLE_DYNAMIC_RESOLVE_VAAPI_SYMBOLS)
    if (QT_FEATURE_vaapi AND NOT DYNAMIC_RESOLVE_VAAPI_SYMBOLS)
        if (FFMPEG_IS_STATIC)
            message(WARNING
                   "QT_FEATURE_vaapi is found but statically built ffmpeg doesn't include vaapi,"
                   "however dynamic symbols resolve is possible.")
        endif()

        set(DYNAMIC_RESOLVE_VAAPI_SYMBOLS TRUE CACHE INTERNAL "")
    elseif (NOT QT_FEATURE_vaapi AND DYNAMIC_RESOLVE_VAAPI_SYMBOLS)

        message(FATAL_ERROR
               "QT_FEATURE_vaapi is not found "
               "but ffmpeg includes VAAPI and dynamic symbols resolve is enabled.")
    endif()
endif()

qt_internal_extend_target(QFFmpegMediaPlugin
    CONDITION
        DYNAMIC_RESOLVE_OPENSSL_SYMBOLS OR DYNAMIC_RESOLVE_VAAPI_SYMBOLS
    SOURCES
        qffmpegsymbolsresolveutils.cpp qffmpegsymbolsresolveutils_p.h
)

function (__propagate_to_compile_definitions VAR)
    if (${VAR})
        target_compile_definitions(QFFmpegMediaPlugin PRIVATE ${VAR})
    endif()
endfunction()

__propagate_to_compile_definitions(DYNAMIC_RESOLVE_OPENSSL_SYMBOLS)
__propagate_to_compile_definitions(DYNAMIC_RESOLVE_VAAPI_SYMBOLS)
__propagate_to_compile_definitions(DYNAMIC_RESOLVE_VA_DRM_SYMBOLS)
__propagate_to_compile_definitions(DYNAMIC_RESOLVE_VA_X11_SYMBOLS)

qt_internal_extend_target(QFFmpegMediaPlugin CONDITION DYNAMIC_RESOLVE_VAAPI_SYMBOLS
    SOURCES
        qffmpegvaapisymbols.cpp
    INCLUDE_DIRECTORIES
        "$<TARGET_PROPERTY:VAAPI::VAAPI,INTERFACE_INCLUDE_DIRECTORIES>"
)

qt_internal_extend_target(QFFmpegMediaPlugin
    CONDITION NOT DYNAMIC_RESOLVE_VAAPI_SYMBOLS AND QT_FEATURE_vaapi
    LIBRARIES VAAPI::VAAPI
)

qt_internal_extend_target(QFFmpegMediaPlugin CONDITION QT_FEATURE_vaapi
    SOURCES
        qffmpeghwaccel_vaapi.cpp qffmpeghwaccel_vaapi_p.h
    NO_UNITY_BUILD_SOURCES
        # Conflicts with macros defined in X11.h, and Xlib.h
        qffmpeghwaccel_vaapi.cpp
    LIBRARIES
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
        ${FWSecurity}
        AVFoundation::AVFoundation
)

qt_internal_extend_target(QFFmpegMediaPlugin CONDITION MACOS
    SOURCES
        qavfscreencapture.mm qavfscreencapture_p.h
        qcgwindowcapture.mm qcgwindowcapture_p.h
        qcgcapturablewindows.mm qcgcapturablewindows_p.h
)

qt_internal_extend_target(QFFmpegMediaPlugin CONDITION WIN32
    SOURCES
        ../windows/qwindowsvideodevices.cpp ../windows/qwindowsvideodevices_p.h
        qwindowscamera.cpp qwindowscamera_p.h
        qffmpeghwaccel_d3d11.cpp qffmpeghwaccel_d3d11_p.h
        qgdiwindowcapture.cpp qgdiwindowcapture_p.h
        qffmpegscreencapture_dxgi.cpp qffmpegscreencapture_dxgi_p.h
        qwincapturablewindows.cpp qwincapturablewindows_p.h
    INCLUDE_DIRECTORIES
        ../windows
    LIBRARIES
        Qt::MultimediaPrivate
        WMF::WMF
        mfreadwrite
)

qt_internal_extend_target(QFFmpegMediaPlugin CONDITION QT_FEATURE_cpp_winrt
    SOURCES
        qffmpegwindowcapture_uwp.cpp qffmpegwindowcapture_uwp_p.h
    INCLUDE_DIRECTORIES
        ../windows
    LIBRARIES
        Dwmapi
        Dxva2
        windowsapp
)

qt_internal_extend_target(QFFmpegMediaPlugin CONDITION QT_FEATURE_xlib
    SOURCES
        qx11surfacecapture.cpp qx11surfacecapture_p.h
        qx11capturablewindows.cpp qx11capturablewindows_p.h
    LIBRARIES
        X11
        Xrandr
        Xext
)

set_source_files_properties(qx11surfacecapture.cpp qx11capturablewindows.cpp # X headers
                            PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)

qt_internal_extend_target(QFFmpegMediaPlugin CONDITION QT_FEATURE_linux_v4l
    SOURCES
        qv4l2camera.cpp qv4l2camera_p.h
        qv4l2filedescriptor.cpp qv4l2filedescriptor_p.h
        qv4l2memorytransfer.cpp qv4l2memorytransfer_p.h
        qv4l2cameradevices.cpp qv4l2cameradevices_p.h
)

if (ANDROID)
    qt_internal_extend_target(QFFmpegMediaPlugin
        SOURCES
            qffmpeghwaccel_mediacodec.cpp qffmpeghwaccel_mediacodec_p.h
            qandroidcamera_p.h qandroidcamera.cpp
            qandroidvideodevices.cpp qandroidvideodevices_p.h
            qandroidcameraframe_p.h  qandroidcameraframe.cpp
            qandroidimagecapture_p.h qandroidimagecapture.cpp
            ../android/wrappers/jni/androidsurfacetexture_p.h
            ../android/wrappers/jni/androidsurfacetexture.cpp
        NO_UNITY_BUILD_SOURCES
            # Duplicate declration of JNI Classes using `Q_DECLARE_JNI_CLASS`
            qandroidcamera.cpp
            qandroidvideodevices.cpp
            qandroidcameraframe.cpp
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
