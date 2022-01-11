qt_find_package(EGL)

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

qt_internal_extend_target(QFFmpegMediaPlugin CONDITION QT_FEATURE_ffmpeg AND NOT QT_FEATURE_pulseaudio
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
