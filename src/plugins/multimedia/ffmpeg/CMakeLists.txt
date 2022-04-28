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
        qffmpegaudiodecoder.cpp qffmpegaudiodecoder_p.h
        qffmpegaudioinput.cpp qffmpegaudioinput_p.h
        qffmpegclock.cpp qffmpegclock_p.h
        qffmpegdecoder.cpp qffmpegdecoder_p.h
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
        qwindowscamera.cpp qwindowscamera_p.h
        qffmpeghwaccel_d3d11.cpp qffmpeghwaccel_d3d11_p.h
    LIBRARIES
        WMF::WMF
        mfreadwrite
)

qt_internal_extend_target(QFFmpegMediaPlugin CONDITION QT_FEATURE_linux_v4l
    SOURCES
        qv4l2camera.cpp qv4l2camera_p.h
)
