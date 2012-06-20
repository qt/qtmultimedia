
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
    qvideoencodersettingscontrol \
    qvideoframe \
    qvideosurfaceformat \
    qwavedecoder \
    qaudiobuffer \
    qdeclarativeaudio \
    qaudiodecoder \
    qaudioprobe \
    qvideoprobe \
    qsamplecache

disabled {
    SUBDIRS += \
        qdeclarativevideo
}
