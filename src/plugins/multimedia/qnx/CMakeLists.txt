
qt_internal_add_plugin(QQnxMediaPlugin
    OUTPUT_NAME qnxmediaplugin
    PLUGIN_TYPE multimedia
    SOURCES
    SOURCES
        audio/qqnxaudioutils.cpp audio/qqnxaudioutils_p.h
        audio/qqnxaudiodevice.cpp audio/qqnxaudiodevice_p.h
        audio/qqnxaudiosink.cpp audio/qqnxaudiosink_p.h
        audio/qqnxaudiosource.cpp audio/qqnxaudiosource_p.h
        camera/qqnxcamera.cpp camera/qqnxcamera_p.h
        camera/qqnximagecapture.cpp camera/qqnximagecapture_p.h
        camera/qqnxmediarecorder.cpp camera/qqnxmediarecorder_p.h
        camera/qqnxmediacapture.cpp camera/qqnxmediacapture_p.h
        common/qqnxwindowgrabber.cpp common/qqnxwindowgrabber_p.h
        common/qqnxaudiooutput.cpp common/qqnxaudiooutput_p.h
        mediaplayer/qqnxmediaplayer.cpp mediaplayer/qqnxmediaplayer_p.h
        mediaplayer/qqnxmediametadata.cpp mediaplayer/qqnxmediametadata_p.h
        mediaplayer/qqnxvideosink.cpp mediaplayer/qqnxvideosink_p.h
        mediaplayer/qqnxmediautil.cpp mediaplayer/qqnxmediautil_p.h
        mediaplayer/qqnxmediaeventthread.cpp mediaplayer/qqnxmediaeventthread_p.h
        qqnxformatinfo.cpp qqnxformatinfo_p.h
        qqnxmediadevices.cpp qqnxmediadevices_p.h
        qqnxmediaintegration.cpp qqnxmediaintegration_p.h
    INCLUDE_DIRECTORIES
        audio
        camera
        common
        mediaplayer
    LIBRARIES
        Qt::MultimediaPrivate
        Qt::CorePrivate
        MMRenderer::MMRenderer
        asound
        camapi
        strm
)
