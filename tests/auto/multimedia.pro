# How do we require a module?
# requires(contains(mobility_modules,multimedia))

TEMPLATE = subdirs
SUBDIRS += \
    qabstractvideobuffer \
    qabstractvideosurface \
    qaudiocapturesource \
    qgraphicsvideoitem \
    qmediaimageviewer \
    qmediaobject \
    qmediaplayer \
    qmediaplayerbackend \
    qmediaplaylist \
    qmediaplaylistnavigator \
    qmediapluginloader \
    qmediarecorder \
    qmediaresource \
    qmediaservice \
    qmediaserviceprovider \
    qmediacontent \
    qradiotuner \
    qcamera \
    qpaintervideosurface \
    qvideowidget \
    qmediatimerange \
    qaudiodeviceinfo \
    qaudiooutput \
    qaudioinput \
    qaudioformat \
    qvideoframe \
    qvideosurfaceformat \
    qcamerabackend

contains (QT_CONFIG, declarative) {
    SUBDIRS += \
#        qsoundeffect \
        qdeclarativeaudio \
        qdeclarativevideo
}

