qt_find_package(EGL)

qt_internal_add_plugin(QFFmpegMediaPlugin
    OUTPUT_NAME ffmegmediaplugin
    PLUGIN_TYPE multimedia
    SOURCES
        qffmpeg_p.h
        qffmpegmediametadata.cpp qffmpegmediametadata_p.h
        qffmpegmediaplayer.cpp qffmpegmediaplayer_p.h
        qffmpegvideosink.cpp qffmpegvideosink_p.h
        qffmpegmediadevices.cpp qffmpegmediadevices_p.h
        qffmpegmediaformatinfo.cpp qffmpegmediaformatinfo_p.h
        qffmpegmediaintegration.cpp qffmpegmediaintegration_p.h
        qffmpegvideobuffer.cpp qffmpegvideobuffer_p.h
    LIBRARIES
        Qt::MultimediaPrivate
        Qt::CorePrivate
        FFmpeg::avcodec FFmpeg::avformat FFmpeg::avutil FFmpeg::avdevice
)
