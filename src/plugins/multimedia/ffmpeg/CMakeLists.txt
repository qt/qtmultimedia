qt_find_package(EGL)
qt_find_package(VAAPI COMPONENTS VA DRM PROVIDED_TARGETS VAAPI::VA VAAPI::DRM MODULE_NAME multimedia QMAKE_LIB vaapi)

qt_internal_find_apple_system_framework(FWCoreMedia CoreMedia) # special case
qt_internal_find_apple_system_framework(FWCoreAudio CoreAudio) # special case
qt_internal_find_apple_system_framework(FWAudioUnit AudioUnit) # special case
qt_internal_find_apple_system_framework(FWVideoToolbox VideoToolbox) # special case
qt_internal_find_apple_system_framework(FWAVFoundation AVFoundation) # special case

qt_internal_add_plugin(QFFmpegMediaPlugin
    OUTPUT_NAME ffmegmediaplugin
    PLUGIN_TYPE multimedia
    SOURCES
        qffmpeg_p.h
        qffmpegdecoder.cpp qffmpegdecoder_p.h
        qffmpeghwaccel.cpp qffmpeghwaccel_p.h
        qffmpegmediametadata.cpp qffmpegmediametadata_p.h
        qffmpegmediaplayer.cpp qffmpegmediaplayer_p.h
        qffmpegvideosink.cpp qffmpegvideosink_p.h
        qffmpegmediaformatinfo.cpp qffmpegmediaformatinfo_p.h
        qffmpegmediaintegration.cpp qffmpegmediaintegration_p.h
        qffmpegvideobuffer.cpp qffmpegvideobuffer_p.h
    LIBRARIES
        Qt::MultimediaPrivate
        Qt::CorePrivate
        FFmpeg::avcodec FFmpeg::avformat FFmpeg::avutil FFmpeg::avdevice FFmpeg::swresample FFmpeg::swscale
)

qt_internal_extend_target(QFFmpegMediaPlugin CONDITION QT_FEATURE_ffmpeg AND QT_FEATURE_vaapi
    SOURCES
        qffmpeghwaccel_vaapi.cpp qffmpeghwaccel_vaapi_p.h
    LIBRARIES
        VAAPI::VAAPI
        EGL::EGL
)

qt_internal_extend_target(QFFmpegMediaPlugin CONDITION QT_FEATURE_ffmpeg AND NOT QT_FEATURE_pulseaudio AND NOT APPLE AND NOT WIN32
    SOURCES
        qffmpegmediadevices.cpp qffmpegmediadevices_p.h
)

qt_internal_extend_target(QFFmpegMediaPlugin CONDITION QT_FEATURE_pulseaudio
    SOURCES
        ../pulseaudio/qpulseaudiodevice.cpp ../pulseaudio/qpulseaudiodevice_p.h
        ../pulseaudio/qaudioengine_pulse.cpp ../pulseaudio/qaudioengine_pulse_p.h
        ../pulseaudio/qpulseaudiosource.cpp ../pulseaudio/qpulseaudiosource_p.h
        ../pulseaudio/qpulseaudiosink.cpp ../pulseaudio/qpulseaudiosink_p.h
        ../pulseaudio/qpulsehelpers.cpp ../pulseaudio/qpulsehelpers_p.h
        ../pulseaudio/qpulseaudiomediadevices.cpp ../pulseaudio/qpulseaudiomediadevices_p.h
    INCLUDE_DIRECTORIES
        ../pulseaudio
    LIBRARIES
        WrapPulseAudio::WrapPulseAudio
)

qt_internal_extend_target(QFFmpegMediaPlugin CONDITION APPLE
    SOURCES
        ../darwin/audio/qdarwinaudiodevice.mm ../darwin/audio/qdarwinaudiodevice_p.h
        ../darwin/audio/qdarwinaudiosource.mm ../darwin/audio/qdarwinaudiosource_p.h
        ../darwin/audio/qdarwinaudiosink.mm ../darwin/audio/qdarwinaudiosink_p.h
        ../darwin/audio/qcoreaudioutils.mm ../darwin/audio/qcoreaudioutils_p.h
        ../darwin/qdarwinmediadevices.mm ../darwin/qdarwinmediadevices_p.h
        qffmpeghwaccel_videotoolbox.mm qffmpeghwaccel_videotoolbox_p.h
    INCLUDE_DIRECTORIES
        ../darwin
        ../darwin/audio
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
        ../windows/audio/qwindowsaudiodevice.cpp ../windows/audio/qwindowsaudiodevice_p.h
        ../windows/audio/qwindowsaudiosink.cpp ../windows/audio/qwindowsaudiosink_p.h
        ../windows/audio/qwindowsaudiosource.cpp ../windows/audio/qwindowsaudiosource_p.h
        ../windows/audio/qwindowsaudioutils.cpp ../windows/audio/qwindowsaudioutils_p.h
        ../windows/common/qwindowsmultimediautils.cpp ../windows/common/qwindowsmultimediautils_p.h
        ../windows/qwindowsmediadevices.cpp ../windows/qwindowsmediadevices_p.h
        ../windows/common/qwindowsmfdefs.cpp ../windows/common/qwindowsmfdefs_p.h
        ../windows/common/qwindowsresampler.cpp ../windows/common/qwindowsresampler_p.h
    INCLUDE_DIRECTORIES
        ../windows
        ../windows/audio
        ../windows/common
    LIBRARIES
        WMF::WMF
        mfreadwrite
        winmm
        wmcodecdspuuid
)
