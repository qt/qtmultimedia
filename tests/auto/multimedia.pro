
TEMPLATE = subdirs
SUBDIRS += \
    qabstractvideobuffer \
    qabstractvideosurface \
    qaudiocapturesource \
    qaudiodeviceinfo \
    qaudioformat \
    qaudioinput \
    qaudiooutput \
    qcamera \
    qcamerabackend \
    qcameraimagecapture \
    qcameraviewfinder \
    qmediabindableinterface \
    qmediacontainercontrol \
    qmediacontent \
    qmediaobject \
    qmediaplayer \
    qmediaplayerbackend \
    qmediaplaylistnavigator \
    qmediarecorder \
    qmediaresource \
    qmediaservice \
    qmediatimerange \
    qradiotuner \
    qvideoframe \
    qvideosurfaceformat \
    qmetadatareadercontrol \
    qmetadatawritercontrol \

# This is disabled because it is unfinished
# qmediastreamscontrol \

# These is disabled until intent is clearer
#    qvideodevicecontrol \
#    qvideoencodercontrol \

# This is a commment for the mock backend directory so that maketestselftest
# doesn't believe it's an untested directory
# qmultimedia_common


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
    qsoundeffect \
    qdeclarativeaudio \


    disabled:SUBDIRS += qdeclarativevideo
  }
}
