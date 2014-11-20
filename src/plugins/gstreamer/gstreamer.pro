TEMPLATE = subdirs

SUBDIRS += \
    audiodecoder \
    mediaplayer \
    mediacapture

config_gstreamer_encodingprofiles {
    SUBDIRS += camerabin
}

OTHER_FILES += \
    gstreamer.json
