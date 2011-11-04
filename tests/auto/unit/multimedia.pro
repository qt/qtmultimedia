
TEMPLATE = subdirs
SUBDIRS += \
    qabstractvideobuffer \
    qabstractvideosurface \
    qaudiocapturesource \
    qaudiodeviceinfo \
    qaudioformat \
    qaudionamespace \
    qcamera \
    qcameraimagecapture \
    qmediabindableinterface \
    qmediacontainercontrol \
    qmediacontent \
    qmediaobject \
    qmediaplayer \
    qmediaplaylistnavigator \
    qmediarecorder \
    qmediaresource \
    qmediaservice \
    qmediatimerange \
    qmetadatareadercontrol \
    qmetadatawritercontrol \
    qradiodata \
    qradiotuner \
    qvideoencodercontrol \
    qvideoframe \
    qvideosurfaceformat \
    qwavedecoder

# Tests depending on private interfaces should only be built if
# these interfaces are exported.
contains (QT_CONFIG, private_tests) {
    # These depend on controlling the set of plugins loaded (in qmediapluginloader)
    SUBDIRS += \
        qdeclarativeaudio \
        qmediaplaylist \
        qmediapluginloader \
        qmediaimageviewer \
        qmediaserviceprovider
}
