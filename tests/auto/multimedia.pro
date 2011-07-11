
TEMPLATE = subdirs
SUBDIRS += \
    qabstractvideobuffer \
    qabstractvideosurface \
    qaudiocapturesource \
    qmediaobject \
    qmediaplayer \
    qmediaplayerbackend \
    qmediaplaylistnavigator \
    qmediarecorder \
    qmediaresource \
    qmediaservice \
    qmediacontent \
    qradiotuner \
    qcamera \
    qmediatimerange \
    qaudiodeviceinfo \
    qaudiooutput \
    qaudioinput \
    qaudioformat \
    qvideoframe \
    qvideosurfaceformat \
    qcamerabackend

# Tests depending on private interfaces should only be built if
# these interfaces are exported.
contains (QT_CONFIG, private_tests) {
  SUBDIRS += \
    qgraphicsvideoitem \
    qmediaimageviewer \
    qmediaplaylist \
    qmediapluginloader \
    qmediaserviceprovider \
    qpaintervideosurface \
    qvideowidget \
}

contains (QT_CONFIG, declarative) {
  # All the declarative tests depend on private interfaces
  contains (QT_CONFIG, private_tests) {
    SUBDIRS += \
#    qsoundeffect \
    qdeclarativeaudio \
    qdeclarativevideo
  }
}

