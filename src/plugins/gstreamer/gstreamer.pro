TEMPLATE = subdirs

SUBDIRS += \
    audiodecoder \
    mediacapture \
    mediaplayer

# Camerabin2 based camera backend is untested and currently disabled
disabled {
    config_gstreamer_photography {
        SUBDIRS += camerabin
    }
}

OTHER_FILES += \
    gstreamer.json
