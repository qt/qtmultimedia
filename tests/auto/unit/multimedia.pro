
TEMPLATE = subdirs
SUBDIRS += \
    qabstractvideobuffer \
    qabstractvideosurface \
    qaudiorecorder \
    qaudioformat \
    qaudionamespace \
    qcamera \
    qcameraimagecapture \
    qmediabindableinterface \
    qmediacontainercontrol \
    qmediacontent \
    qmediaobject \
    qmediaplayer \
    qmediaplaylist \
    qmediaplaylistnavigator \
    qmediapluginloader \
    qmediarecorder \
    qmediaresource \
    qmediaservice \
    qmediaserviceprovider \
    qmediatimerange \
    qmetadatareadercontrol \
    qmetadatawritercontrol \
    qradiodata \
    qradiotuner \
    qvideoencodercontrol \
    qvideoframe \
    qvideosurfaceformat \
    qwavedecoder \

# Tests depending on private interfaces should only be built if
# these interfaces are exported.
contains (QT_CONFIG, private_tests) {
    SUBDIRS += \
        qdeclarativeaudio \
        qmediaimageviewer
}
