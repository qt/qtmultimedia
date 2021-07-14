TEMPLATE = subdirs
QT_FOR_CONFIG += multimedia-private

SUBDIRS += audiodecoder

# These examples all need widgets for now (using creator templates that use widgets)
qtHaveModule(widgets) {
    SUBDIRS += \
        spectrum \
        audiorecorder \
        audiodevices \
        audiooutput
}

qtHaveModule(quick) {
    SUBDIRS += \
        declarative-camera \
        video
}
