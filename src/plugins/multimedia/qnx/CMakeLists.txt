
qt_internal_add_plugin(QQnxMediaPlugin
    OUTPUT_NAME qnxmediaplugin
    PLUGIN_TYPE multimedia
    SOURCES
    SOURCES
        audio/qqnxaudioutils.cpp audio/qqnxaudioutils_p.h
        audio/qqnxaudiodevice.cpp audio/qqnxaudiodevice_p.h
        audio/qqnxaudioinput.cpp audio/qqnxaudioinput_p.h
        audio/qqnxaudiooutput.cpp audio/qqnxaudiooutput_p.h
        audio/qqnxaudiosink.cpp audio/qqnxaudiosink_p.h
        audio/qqnxaudiosource.cpp audio/qqnxaudiosource_p.h
        camera/qqnxcamera.cpp camera/qqnxcamera_p.h
        camera/qqnximagecapture.cpp camera/qqnximagecapture_p.h
        camera/qqnxmediarecorder.cpp camera/qqnxmediarecorder_p.h
        common/qqnxmediaeventthread.cpp common/qqnxmediaeventthread_p.h
        common/qqnxwindowgrabber.cpp common/qqnxwindowgrabber_p.h
        capture/qqnxaudiorecorder.cpp capture/qqnxaudiorecorder_p.h
        capture/qqnxmediacapturesession.cpp capture/qqnxmediacapturesession_p.h
        mediaplayer/qqnxmediaplayer.cpp mediaplayer/qqnxmediaplayer_p.h
        mediaplayer/qqnxmediametadata.cpp mediaplayer/qqnxmediametadata_p.h
        mediaplayer/qqnxvideosink.cpp mediaplayer/qqnxvideosink_p.h
        mediaplayer/qqnxmediautil.cpp mediaplayer/qqnxmediautil_p.h
        qqnxformatinfo.cpp qqnxformatinfo_p.h
        qqnxmediadevices.cpp qqnxmediadevices_p.h
        qqnxmediaintegration.cpp qqnxmediaintegration_p.h
    INCLUDE_DIRECTORIES
        audio
        camera
        capture
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
