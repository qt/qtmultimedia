TEMPLATE = subdirs

SUBDIRS += \
    audiodecoder \
    mediacapture \
    mediaplayer

# Camerabin2 based camera backend is untested and currently disabled
disabled {
    contains(config_test_gstreamer_photography, yes) {
        SUBDIRS += camerabin
    }
}

OTHER_FILES += \
    gstreamer.json
