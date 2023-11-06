# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from multimedia.pro.

#####################################################################
## Multimedia Module:
#####################################################################

find_package(Qt6 COMPONENTS ShaderTools)

qt_internal_find_apple_system_framework(FWCoreAudio CoreAudio) # special case
qt_internal_find_apple_system_framework(FWCoreVideo CoreVideo) # special case
qt_internal_find_apple_system_framework(FWAudioUnit AudioUnit) # special case
qt_internal_find_apple_system_framework(FWCoreMedia CoreMedia) # special case
qt_internal_find_apple_system_framework(FWAudioToolbox AudioToolbox) # special case

qt_internal_add_module(Multimedia
    PLUGIN_TYPES multimedia
    SOURCES
        audio/qaudio.cpp audio/qaudio.h
        audio/qaudiobuffer.cpp audio/qaudiobuffer.h
        audio/qaudiodecoder.cpp audio/qaudiodecoder.h
        audio/qaudiodevice.cpp audio/qaudiodevice.h audio/qaudiodevice_p.h
        audio/qaudioinput.cpp audio/qaudioinput.h
        audio/qaudiooutput.cpp audio/qaudiooutput.h
        audio/qaudioformat.cpp audio/qaudioformat.h
        audio/qaudiohelpers.cpp audio/qaudiohelpers_p.h
        audio/qaudiosource.cpp audio/qaudiosource.h
        audio/qaudiosink.cpp audio/qaudiosink.h
        audio/qaudiosystem.cpp audio/qaudiosystem_p.h
        audio/qaudiostatemachine.cpp audio/qaudiostatemachine_p.h
        audio/qaudiostatemachineutils_p.h
        audio/qsamplecache_p.cpp audio/qsamplecache_p.h
        audio/qsoundeffect.cpp audio/qsoundeffect.h
        audio/qwavedecoder.cpp audio/qwavedecoder.h
        camera/qcamera.cpp camera/qcamera.h camera/qcamera_p.h
        camera/qcameradevice.cpp camera/qcameradevice.h camera/qcameradevice_p.h
        camera/qimagecapture.cpp camera/qimagecapture.h
        platform/qplatformaudiodecoder.cpp platform/qplatformaudiodecoder_p.h
        platform/qplatformaudioinput_p.h
        platform/qplatformaudiooutput_p.h
        platform/qplatformcamera.cpp platform/qplatformcamera_p.h
        platform/qplatformvideosource.cpp platform/qplatformvideosource_p.h
        platform/qplatformsurfacecapture.cpp platform/qplatformsurfacecapture_p.h
        platform/qplatformimagecapture.cpp platform/qplatformimagecapture_p.h
        platform/qplatformmediacapture.cpp platform/qplatformmediacapture_p.h
        platform/qplatformmediadevices.cpp platform/qplatformmediadevices_p.h
        platform/qplatformmediarecorder.cpp platform/qplatformmediarecorder_p.h
        platform/qplatformmediaformatinfo.cpp  platform/qplatformmediaformatinfo_p.h
        platform/qplatformmediaintegration.cpp platform/qplatformmediaintegration_p.h
        platform/qplatformmediaplayer.cpp platform/qplatformmediaplayer_p.h
        platform/qplatformmediaplugin.cpp platform/qplatformmediaplugin_p.h
        platform/qplatformvideodevices.cpp platform/qplatformvideodevices_p.h
        platform/qplatformvideosink.cpp platform/qplatformvideosink_p.h
        playback/qmediaplayer.cpp playback/qmediaplayer.h playback/qmediaplayer_p.h
        platform/qplatformcapturablewindows_p.h
        qmediadevices.cpp qmediadevices.h
        qmediaenumdebug.h
        qmediaformat.cpp  qmediaformat.h
        qmediametadata.cpp qmediametadata.h
        qmediastoragelocation.cpp qmediastoragelocation_p.h
        qmediatimerange.cpp qmediatimerange.h
        qmultimediautils.cpp qmultimediautils_p.h
        qtmultimediaglobal.h qtmultimediaglobal_p.h
        qerrorinfo_p.h
        recording/qmediacapturesession.cpp recording/qmediacapturesession.h
        recording/qmediarecorder.cpp recording/qmediarecorder.h recording/qmediarecorder_p.h
        recording/qscreencapture.cpp recording/qscreencapture.h
        recording/qwindowcapture.cpp recording/qwindowcapture.h
        recording/qcapturablewindow.cpp recording/qcapturablewindow.h recording/qcapturablewindow_p.h
        video/qabstractvideobuffer.cpp video/qabstractvideobuffer_p.h
        video/qmemoryvideobuffer.cpp video/qmemoryvideobuffer_p.h
        video/qvideoframe.cpp video/qvideoframe.h
        video/qvideosink.cpp video/qvideosink.h
        video/qvideotexturehelper.cpp video/qvideotexturehelper_p.h
        video/qvideoframeconversionhelper.cpp video/qvideoframeconversionhelper_p.h
        video/qvideooutputorientationhandler.cpp video/qvideooutputorientationhandler_p.h
        video/qvideoframeconverter.cpp video/qvideoframeconverter_p.h
        video/qvideoframeformat.cpp video/qvideoframeformat.h
        video/qvideowindow.cpp video/qvideowindow_p.h
    INCLUDE_DIRECTORIES
        audio
        camera
        controls
        platform
        playback
        recording
        video
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Network
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
        Qt::GuiPrivate
    GENERATE_CPP_EXPORTS
)

qt_internal_add_simd_part(Multimedia SIMD sse2
    SOURCES
        video/qvideoframeconversionhelper_sse2.cpp
)

qt_internal_add_simd_part(Multimedia SIMD ssse3
    SOURCES
        video/qvideoframeconversionhelper_ssse3.cpp
)

qt_internal_add_simd_part(Multimedia SIMD arch_haswell
    SOURCES
        video/qvideoframeconversionhelper_avx2.cpp
    EXCLUDE_OSX_ARCHITECTURES
        arm64
)

qt_internal_add_docs(Multimedia
    doc/qtmultimedia.qdocconf
)

qt_internal_extend_target(Multimedia CONDITION QT_FEATURE_alsa
    SOURCES
        alsa/qalsaaudiodevice.cpp alsa/qalsaaudiodevice_p.h
        alsa/qalsaaudiosource.cpp alsa/qalsaaudiosource_p.h
        alsa/qalsaaudiosink.cpp alsa/qalsaaudiosink_p.h
        alsa/qalsamediadevices.cpp alsa/qalsamediadevices_p.h
    INCLUDE_DIRECTORIES
        alsa
    LIBRARIES
        ALSA::ALSA
)

qt_internal_extend_target(Multimedia CONDITION QT_FEATURE_pulseaudio
    SOURCES
        pulseaudio/qpulseaudiodevice.cpp pulseaudio/qpulseaudiodevice_p.h
        pulseaudio/qaudioengine_pulse.cpp pulseaudio/qaudioengine_pulse_p.h
        pulseaudio/qpulseaudiosource.cpp pulseaudio/qpulseaudiosource_p.h
        pulseaudio/qpulseaudiosink.cpp pulseaudio/qpulseaudiosink_p.h
        pulseaudio/qpulsehelpers.cpp pulseaudio/qpulsehelpers_p.h
        pulseaudio/qpulseaudiomediadevices.cpp pulseaudio/qpulseaudiomediadevices_p.h
    INCLUDE_DIRECTORIES
        pulseaudio
    LIBRARIES
        WrapPulseAudio::WrapPulseAudio
)

qt_internal_extend_target(Multimedia CONDITION ANDROID
    SOURCES
        android/qandroidaudiosource.cpp android/qandroidaudiosource_p.h
        android/qandroidaudiosink.cpp android/qandroidaudiosink_p.h
        android/qandroidaudiodevice.cpp android/qandroidaudiodevice_p.h
        android/qopenslesengine.cpp android/qopenslesengine_p.h
        android/qandroidmediadevices.cpp android/qandroidmediadevices_p.h
    INCLUDE_DIRECTORIES
        android
    LIBRARIES
        OpenSLES
)

if (ANDROID)
    set_property(TARGET Multimedia APPEND PROPERTY QT_ANDROID_BUNDLED_JAR_DEPENDENCIES
        jar/Qt${QtMultimedia_VERSION_MAJOR}AndroidMultimedia.jar:org.qtproject.qt.android.multimedia.QtAudioDeviceManager
    )
endif()

qt_internal_extend_target(Multimedia CONDITION APPLE
    SOURCES
        darwin/qdarwinaudiodevice.mm darwin/qdarwinaudiodevice_p.h
        darwin/qdarwinaudiosource.mm darwin/qdarwinaudiosource_p.h
        darwin/qdarwinaudiosink.mm darwin/qdarwinaudiosink_p.h
        darwin/qcoreaudioutils.mm darwin/qcoreaudioutils_p.h
        darwin/qdarwinmediadevices.mm darwin/qdarwinmediadevices_p.h
    INCLUDE_DIRECTORIES
        darwin
    LIBRARIES
        ${FWAudioToolbox}
        ${FWCoreAudio}
        ${FWCoreMedia}
        ${FWCoreFoundation}
)

qt_internal_extend_target(Multimedia CONDITION MACOS
    SOURCES
        darwin/qmacosaudiodatautils_p.h
)

qt_internal_extend_target(Multimedia CONDITION IOS OR TVOS
    SOURCES
        darwin/qcoreaudiosessionmanager.mm darwin/qcoreaudiosessionmanager_p.h
    LIBRARIES
        ${FWAVFoundation}
        ${FWFoundation}
)

qt_internal_extend_target(Multimedia CONDITION QNX
    SOURCES
        qnx/qqnxaudiodevice.cpp qnx/qqnxaudiodevice_p.h
        qnx/qqnxaudiosink.cpp qnx/qqnxaudiosink_p.h
        qnx/qqnxaudiosource.cpp qnx/qqnxaudiosource_p.h
        qnx/qqnxaudioutils.cpp qnx/qqnxaudioutils_p.h
        qnx/qqnxmediadevices.cpp qnx/qqnxmediadevices_p.h
    INCLUDE_DIRECTORIES
        qnx
    LIBRARIES
        asound
)

qt_internal_extend_target(Multimedia CONDITION QT_FEATURE_wmf
    SOURCES
        windows/qwindowsaudiodevice.cpp windows/qwindowsaudiodevice_p.h
        windows/qwindowsaudiosource.cpp windows/qwindowsaudiosource_p.h
        windows/qwindowsaudiosink.cpp windows/qwindowsaudiosink_p.h
        windows/qwindowsaudioutils.cpp windows/qwindowsaudioutils_p.h
        windows/qwindowsmediadevices.cpp windows/qwindowsmediadevices_p.h
        windows/qwindowsmediafoundation.cpp windows/qwindowsmediafoundation_p.h
        windows/qwindowsresampler.cpp windows/qwindowsresampler_p.h
        windows/qwindowsmultimediautils.cpp windows/qwindowsmultimediautils_p.h
        windows/qwindowsmfdefs.cpp windows/qwindowsmfdefs_p.h
        windows/qcomptr_p.h
        windows/qcomtaskresource_p.h
    INCLUDE_DIRECTORIES
        windows
    LIBRARIES
        winmm
        ksuser
)

# Only on Windows' MinGW/LLVM build, symbols from `strmif.h` and `ddraw.h` conflicts
# with symbols defined in `ksmedia.h`.
qt_internal_extend_target(Multimedia CONDITION QT_FEATURE_wmf AND MINGW
    NO_UNITY_BUILD_SOURCES
        windows/qwindowsaudiodevice.cpp windows/qwindowsaudiodevice_p.h
        windows/qwindowsaudiosource.cpp windows/qwindowsaudiosource_p.h
        windows/qwindowsaudiosink.cpp windows/qwindowsaudiosink_p.h
        windows/qwindowsaudioutils.cpp windows/qwindowsaudioutils_p.h
        windows/qwindowsmediadevices.cpp windows/qwindowsmediadevices_p.h
        windows/qwindowsmediafoundation.cpp windows/qwindowsmediafoundation_p.h
        windows/qwindowsresampler.cpp windows/qwindowsresampler_p.h
        windows/qwindowsmultimediautils.cpp windows/qwindowsmultimediautils_p.h
        windows/qwindowsmfdefs.cpp windows/qwindowsmfdefs_p.h
        windows/qcomptr_p.h
        windows/qcomtaskresource_p.h
)

qt_internal_extend_target(Multimedia CONDITION WASM
    SOURCES
        wasm/qwasmmediadevices.cpp wasm/qwasmmediadevices_p.h
        wasm/qwasmaudiosource.cpp wasm/qwasmaudiosource_p.h
        wasm/qwasmaudiosink.cpp wasm/qwasmaudiosink_p.h
        wasm/qwasmaudiodevice.cpp wasm/qwasmaudiodevice_p.h
    INCLUDE_DIRECTORIES
        wasm
    PUBLIC_LIBRARIES
        openal
    NO_UNITY_BUILD_SOURCES
        wasm/qwasmaudiosink.cpp
        # To avoid collision between symbols defined in wasm/qwasmaudiosource.cpp.
)

set(VIDEO_VERTEX_SHADERS
    "shaders/vertex.vert"
    "shaders/externalsampler.vert"
)

set(VIDEO_SHADERS
    "shaders/externalsampler.frag@glsl,100es,shaders/externalsampler_gles.frag"
    "shaders/abgr.frag"
    "shaders/argb.frag"
    "shaders/rgba.frag"
    "shaders/bgra.frag"
    "shaders/y.frag"
    "shaders/nv12.frag"
    "shaders/nv21.frag"
    "shaders/imc2.frag"
    "shaders/imc4.frag"
    "shaders/uyvy.frag"
    "shaders/yuv_triplanar.frag"
    "shaders/yuv_triplanar_p10.frag"
    "shaders/yvu_triplanar.frag"
    "shaders/yuyv.frag"
    "shaders/ayuv.frag"
    "shaders/nv12_bt2020_pq.frag"
    "shaders/nv12_bt2020_hlg.frag"
)

qt_internal_add_shaders(Multimedia "qtmultimedia_shaders"
    SILENT
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    PREFIX
        "/qt-project.org/multimedia"
    FILES
        ${VIDEO_VERTEX_SHADERS}
        ${VIDEO_SHADERS}
)

string(REPLACE ".frag" "_linear.frag.qsb" LINEAR_VIDEO_SHADERS "${VIDEO_SHADERS}")

qt_internal_add_shaders(Multimedia "qtmultimedia_shaders_linear"
    SILENT
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    PREFIX
        "/qt-project.org/multimedia"
    FILES
        ${VIDEO_SHADERS}
    OUTPUTS
        ${LINEAR_VIDEO_SHADERS}
    DEFINES
        QMM_OUTPUTSURFACE_LINEAR
)

qt_internal_add_shaders(Multimedia "qtmultimedia_shaders_gl_macos"
    SILENT
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    PREFIX
        "/qt-project.org/multimedia"
    GLSL
        "120,150"
    NOHLSL
    NOMSL
    FILES
        "shaders/rectsampler.vert"
        "shaders/rectsampler_bgra.frag"
)

qt_internal_add_shaders(Multimedia "qtmultimedia_shaders_gl_macos_linear"
    SILENT
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    PREFIX
        "/qt-project.org/multimedia"
    GLSL
        "120,150"
    NOHLSL
    NOMSL
    FILES
        "shaders/rectsampler_bgra.frag"
    OUTPUTS
        "shaders/rectsampler_bgra_linear.frag.qsb"
)

if (DEFINED QT_DEFAULT_MEDIA_BACKEND)
    target_compile_definitions(Multimedia
        PRIVATE QT_DEFAULT_MEDIA_BACKEND="${QT_DEFAULT_MEDIA_BACKEND}")
endif()
