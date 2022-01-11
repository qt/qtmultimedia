qt_find_package(EGL)

qt_internal_add_plugin(QFFmpegMediaPlugin
    OUTPUT_NAME ffmegmediaplugin
    PLUGIN_TYPE multimedia
    SOURCES
        qffmpegmediadevices.cpp qffmpegmediadevices_p.h
        qffmpegmediaformatinfo.cpp qffmpegmediaformatinfo_p.h
        qffmpegmediaintegration.cpp qffmpegmediaintegration_p.h
    LIBRARIES
        Qt::MultimediaPrivate
        Qt::CorePrivate
        FFmpeg::avcodec FFmpeg::avformat FFmpeg::avutil FFmpeg::avdevice
)
