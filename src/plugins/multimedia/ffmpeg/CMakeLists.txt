# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_find_package(EGL)
qt_find_package(VAAPI COMPONENTS VA DRM PROVIDED_TARGETS VAAPI::VAAPI MODULE_NAME multimedia QMAKE_LIB vaapi)

qt_internal_add_module(FFmpegMediaPluginImplPrivate
    STATIC
    INTERNAL_MODULE
    SOURCES
        qffmpeg.cpp qffmpeg_p.h
        qffmpegdefs_p.h
        qffmpegcodecstorage.cpp qffmpegcodecstorage_p.h
        qffmpegioutils.cpp qffmpegioutils_p.h
        qffmpegavaudioformat.cpp qffmpegavaudioformat_p.h
        qffmpegaudiodecoder.cpp qffmpegaudiodecoder_p.h
        qffmpegaudioinput.cpp qffmpegaudioinput_p.h
        qffmpegcodec.cpp qffmpegcodec_p.h
        qffmpegconverter.cpp qffmpegconverter_p.h
        qffmpeghwaccel.cpp qffmpeghwaccel_p.h
        qffmpegtextureconverter.cpp qffmpegtextureconverter_p.h
        qffmpegmediametadata.cpp qffmpegmediametadata_p.h
        qffmpegmediaplayer.cpp qffmpegmediaplayer_p.h
        qffmpegvideosink.cpp qffmpegvideosink_p.h
        qffmpegmediaformatinfo.cpp qffmpegmediaformatinfo_p.h
        qffmpegmediaintegration.cpp qffmpegmediaintegration_p.h
        qffmpegvideobuffer.cpp qffmpegvideobuffer_p.h
        qffmpegimagecapture.cpp qffmpegimagecapture_p.h
        qffmpegmediacapturesession.cpp qffmpegmediacapturesession_p.h
        qffmpegmediarecorder.cpp qffmpegmediarecorder_p.h
        qffmpegthread.cpp qffmpegthread_p.h
        qffmpegresampler.cpp qffmpegresampler_p.h
        qffmpegencodingformatcontext.cpp qffmpegencodingformatcontext_p.h
        qgrabwindowsurfacecapture.cpp qgrabwindowsurfacecapture_p.h
        qffmpegsurfacecapturegrabber.cpp qffmpegsurfacecapturegrabber_p.h

        qffmpegplaybackengine.cpp qffmpegplaybackengine_p.h
        playbackengine/qffmpegplaybackenginedefs_p.h
        playbackengine/qffmpegplaybackengineobject.cpp playbackengine/qffmpegplaybackengineobject_p.h
        playbackengine/qffmpegdemuxer.cpp playbackengine/qffmpegdemuxer_p.h
        playbackengine/qffmpegstreamdecoder.cpp playbackengine/qffmpegstreamdecoder_p.h
        playbackengine/qffmpegrenderer.cpp playbackengine/qffmpegrenderer_p.h
        playbackengine/qffmpegaudioframeconverter.cpp playbackengine/qffmpegaudioframeconverter_p.h
        playbackengine/qffmpegaudiorenderer.cpp playbackengine/qffmpegaudiorenderer_p.h
        playbackengine/qffmpegvideorenderer.cpp playbackengine/qffmpegvideorenderer_p.h
        playbackengine/qffmpegsubtitlerenderer.cpp playbackengine/qffmpegsubtitlerenderer_p.h
        playbackengine/qffmpegtime_p.h
        playbackengine/qffmpegtimecontroller.cpp playbackengine/qffmpegtimecontroller_p.h
        playbackengine/qffmpegmediadataholder.cpp playbackengine/qffmpegmediadataholder_p.h
        playbackengine/qffmpegcodeccontext.cpp playbackengine/qffmpegcodeccontext_p.h
        playbackengine/qffmpegpacket_p.h
        playbackengine/qffmpegframe_p.h
        playbackengine/qffmpegplaybackutils_p.h

        recordingengine/qffmpegaudioencoder_p.h
        recordingengine/qffmpegaudioencoder.cpp
        recordingengine/qffmpegaudioencoderutils_p.h
        recordingengine/qffmpegaudioencoderutils.cpp
        recordingengine/qffmpegencoderthread_p.h
        recordingengine/qffmpegencoderthread.cpp
        recordingengine/qffmpegencoderoptions_p.h
        recordingengine/qffmpegencoderoptions.cpp
        recordingengine/qffmpegmuxer_p.h
        recordingengine/qffmpegmuxer.cpp
        recordingengine/qffmpegrecordingengine_p.h
        recordingengine/qffmpegrecordingengine.cpp
        recordingengine/qffmpegencodinginitializer_p.h
        recordingengine/qffmpegencodinginitializer.cpp
        recordingengine/qffmpegrecordingengineutils_p.h
        recordingengine/qffmpegrecordingengineutils.cpp
        recordingengine/qffmpegvideoencoder_p.h
        recordingengine/qffmpegvideoencoder.cpp
        recordingengine/qffmpegvideoencoderutils_p.h
        recordingengine/qffmpegvideoencoderutils.cpp
        recordingengine/qffmpegvideoframeencoder_p.h
        recordingengine/qffmpegvideoframeencoder.cpp
    NO_GENERATE_CPP_EXPORTS
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    PUBLIC_LIBRARIES
        Qt::MultimediaPrivate
        Qt::CorePrivate
    NO_PCH_SOURCES
        # compiled with different compiler options:
        # not used because `__NO_INLINE__' not defined [-Werror=invalid-pch]
        playbackengine/qffmpegaudioframeconverter.cpp

    SYSTEM_INCLUDE_DIRECTORIES
        ../../../3rdparty/signalsmith-stretch/
    ATTRIBUTION_FILE_DIR_PATHS
        ../../../3rdparty/signalsmith-stretch
)

if (LINUX OR ANDROID)
    # We have 2 options: link shared stubs to FFmpegMediaPluginImplPrivate vs
    # static compilation of the needed stubs to the FFmpeg plugin.
    # Currently, we chose the second option so that user could trivially
    # remove the FFmpeg libs we ship.
    # Set QT_LINK_STUBS_TO_FFMPEG_PLUGIN = TRUE to change the behavior.

    # set(QT_LINK_STUBS_TO_FFMPEG_PLUGIN TRUE)

    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/QtAddFFmpegStubs.cmake")
    qt_internal_multimedia_add_ffmpeg_stubs()
endif()

if (QT_FEATURE_vaapi)
    qt_internal_extend_target(FFmpegMediaPluginImplPrivate
        SOURCES
            qffmpeghwaccel_vaapi.cpp qffmpeghwaccel_vaapi_p.h
        NO_UNITY_BUILD_SOURCES
            # Conflicts with macros defined in X11.h, and Xlib.h
            qffmpeghwaccel_vaapi.cpp
        LIBRARIES
            EGL::EGL
    )

    list(FIND FFMPEG_STUBS "va" va_stub_index)
    if (NOT QT_LINK_STUBS_TO_FFMPEG_PLUGIN AND (FFMPEG_SHARED_LIBRARIES OR ${va_stub_index} EQUAL -1))
        target_compile_definitions(FFmpegMediaPluginImplPrivate PRIVATE Q_FFMPEG_PLUGIN_STUBS_ONLY)
        qt_internal_multimedia_find_vaapi_soversion()
        qt_internal_multimedia_add_private_stub_to_plugin("va")
    endif()
endif()


qt_internal_extend_target(FFmpegMediaPluginImplPrivate CONDITION APPLE
    SOURCES
        qffmpeghwaccel_videotoolbox.mm qffmpeghwaccel_videotoolbox_p.h
        darwin/qavfcamera.mm darwin/qavfcamera_p.h
        darwin/qavfsamplebufferdelegate.mm darwin/qavfsamplebufferdelegate_p.h

    NO_UNITY_BUILD_SOURCES
        qffmpeghwaccel_videotoolbox.mm # AVMediaType clash between libavformat and AVFoundation

    LIBRARIES
        ${FWAudioToolbox}
        ${FWAVFoundation}
        ${FWCoreMedia}
        ${FWCoreVideo}
        ${FWSecurity}
)

qt_internal_extend_target(FFmpegMediaPluginImplPrivate CONDITION MACOS
    SOURCES
        darwin/qavfscreencapture.mm darwin/qavfscreencapture_p.h
        darwin/qcgwindowcapture.mm darwin/qcgwindowcapture_p.h
        darwin/qcgcapturablewindows.mm darwin/qcgcapturablewindows_p.h
    LIBRARIES
        ${FWAppKit}
)

qt_internal_extend_target(FFmpegMediaPluginImplPrivate CONDITION WIN32
    SOURCES
        qwindowscamera.cpp qwindowscamera_p.h
        qffmpeghwaccel_d3d11.cpp qffmpeghwaccel_d3d11_p.h
        qgdiwindowcapture.cpp qgdiwindowcapture_p.h
        qffmpegscreencapture_dxgi.cpp qffmpegscreencapture_dxgi_p.h
        qwincapturablewindows.cpp qwincapturablewindows_p.h
    LIBRARIES
        Qt::MultimediaPrivate
        mf
        mfplat
        mfreadwrite
        mfuuid
)

qt_internal_extend_target(FFmpegMediaPluginImplPrivate CONDITION QT_FEATURE_cpp_winrt
    SOURCES
        qffmpegwindowcapture_uwp.cpp qffmpegwindowcapture_uwp_p.h
    LIBRARIES
        Dwmapi
        Dxva2
        windowsapp
)

if(QT_FEATURE_cpp_winrt)
    qt_internal_set_exceptions_flags(FFmpegMediaPluginImplPrivate ON)
endif()

qt_internal_extend_target(FFmpegMediaPluginImplPrivate CONDITION QT_FEATURE_xlib
    SOURCES
        qx11surfacecapture.cpp qx11surfacecapture_p.h
        qx11capturablewindows.cpp qx11capturablewindows_p.h
    LIBRARIES
        X11
        Xrandr
        Xext
)


if(QT_FEATURE_eglfs AND TARGET Qt::OpenGLPrivate)
    if(TARGET Qt::Quick)
        qt_internal_extend_target(FFmpegMediaPluginImplPrivate
            LIBRARIES Qt::Quick
        )
    else()
        # There's no "quick" feature we could test for in C++ code. Add a define for that.
        qt_internal_extend_target(FFmpegMediaPluginImplPrivate
            DEFINES QT_NO_QUICK
        )
    endif()

    qt_internal_extend_target(FFmpegMediaPluginImplPrivate
        SOURCES
            qeglfsscreencapture.cpp qeglfsscreencapture_p.h
            qopenglvideobuffer.cpp qopenglvideobuffer_p.h
        LIBRARIES
            Qt::OpenGLPrivate
    )
endif()

set_source_files_properties(qx11surfacecapture.cpp qx11capturablewindows.cpp # X headers
                            PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)

qt_internal_extend_target(FFmpegMediaPluginImplPrivate CONDITION QT_FEATURE_linux_v4l
    SOURCES
        qv4l2camera.cpp qv4l2camera_p.h
        qv4l2filedescriptor.cpp qv4l2filedescriptor_p.h
        qv4l2memorytransfer.cpp qv4l2memorytransfer_p.h
        qv4l2cameradevices.cpp qv4l2cameradevices_p.h
)

if (ANDROID)
    qt_internal_extend_target(FFmpegMediaPluginImplPrivate
        SOURCES
            qffmpeghwaccel_mediacodec.cpp qffmpeghwaccel_mediacodec_p.h
            qandroidcamera_p.h qandroidcamera.cpp
            qandroidvideodevices.cpp qandroidvideodevices_p.h
            qandroidvideoframebuffer_p.h  qandroidvideoframebuffer.cpp
            qandroidvideoframefactory_p.h qandroidvideoframefactory.cpp
            qandroidimagecapture_p.h qandroidimagecapture.cpp
            qandroidscreencapture_p.h qandroidscreencapture.cpp
            ../android/wrappers/jni/androidsurfacetexture_p.h
            ../android/wrappers/jni/androidsurfacetexture.cpp
        NO_UNITY_BUILD_SOURCES
            # Duplicate declration of JNI Classes using `Q_DECLARE_JNI_CLASS`
            qandroidcamera.cpp
            qandroidvideodevices.cpp
            qandroidvideoframebuffer.cpp
        INCLUDE_DIRECTORIES
            ${FFMPEG_DIR}/include
            ../android/wrappers/jni/
        LIBRARIES
            mediandk
            android
    )
endif()

# signalsmith-stretch is header-only and rather slow in debug builds. so let's optimize it:
if(GCC OR CLANG)
    set_source_files_properties(playbackengine/qffmpegaudioframeconverter.cpp PROPERTIES
        COMPILE_OPTIONS $<$<CONFIG:Debug>:-O3>)
elseif(MSVC)
    # we cannot just /O2, so we need to rely on pragma optimize here
    set_source_files_properties(playbackengine/qffmpegaudioframeconverter.cpp PROPERTIES
        COMPILE_DEFINITIONS $<$<CONFIG:Debug>:QT_MM_OPTIMIZE_DEBUG>
    )
endif()


# TODO: get libs from FindFFmpeg.cmake
set(ffmpeg_libs FFmpeg::avformat FFmpeg::avcodec FFmpeg::swresample FFmpeg::swscale FFmpeg::avutil)

if (QT_DEPLOY_FFMPEG AND NOT BUILD_SHARED_LIBS AND NOT UIKIT)
    message(FATAL_ERROR "QT_DEPLOY_FFMPEG is not implemented yet for static builds")
endif()

if (QT_DEPLOY_FFMPEG AND FFMPEG_SHARED_LIBRARIES AND (BUILD_SHARED_LIBS OR UIKIT))
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/QtDeployFFmpeg.cmake")
    qt_internal_multimedia_copy_or_install_ffmpeg()
endif()

if( NOT (UIKIT AND FFMPEG_SHARED_LIBRARIES) )
    qt_internal_extend_target(FFmpegMediaPluginImplPrivate PUBLIC_LIBRARIES ${ffmpeg_libs})
else()
    # applications need to link against the ffmpeg libs via qt_add_ios_ffmpeg_libraries
    foreach(ffmpeg_lib IN LISTS ffmpeg_libs)
        qt_internal_add_target_include_dirs(FFmpegMediaPluginImplPrivate ${ffmpeg_lib})
        target_include_directories(FFmpegMediaPluginImplPrivate PUBLIC
            "$<TARGET_PROPERTY:${ffmpeg_lib},INTERFACE_COMPILE_DEFINITIONS>")
    endforeach()
endif()

qt_internal_add_plugin(QFFmpegMediaPlugin
    OUTPUT_NAME ffmpegmediaplugin
    PLUGIN_TYPE multimedia
    SOURCES
        qffmpegplugin.cpp
        ffmpeg.json
    LIBRARIES
        Qt::FFmpegMediaPluginImplPrivate
        Qt::MultimediaPrivate
)

if (ANDROID)
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
