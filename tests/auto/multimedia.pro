
TEMPLATE = subdirs
SUBDIRS += \
    qabstractvideobuffer \
    qabstractvideosurface \
    qaudiocapturesource \
    qaudiodeviceinfo \
    qaudioformat \
    qaudioinput \
    qaudiooutput \
    qmediabindableinterface \
    qmediacontainercontrol \
    qmediacontent \
    qmediaplayerbackend \
    qmediaplaylistnavigator \
    qmediarecorder \
    qmediaresource \
    qmediaservice \
    qmediatimerange \
    qradiotuner \
    qradiodata \
    qvideoframe \
    qvideosurfaceformat \
    qmetadatareadercontrol \
    qmetadatawritercontrol \
    qmediaplayer \
    qcameraimagecapture \
    qmediaobject \
    qcamera \
    qcamerabackend \
    qwavedecoder

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
        qmediaplaylist \
        qmediapluginloader \
        qmediaimageviewer \
        qmediaserviceprovider

    contains (QT_CONFIG, declarative) {
  # All the declarative tests depend on private interfaces
        SUBDIRS += \
            qsoundeffect \
            qdeclarativeaudio
    }
}
