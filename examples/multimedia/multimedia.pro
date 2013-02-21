TEMPLATE = subdirs

SUBDIRS += audiodecoder

# These examples all need widgets for now (using creator templates that use widgets)
qtHaveModule(widgets) {
    SUBDIRS += \
        radio \
        spectrum \
        audiorecorder \
        audiodevices \
        audioinput \
        audiooutput \
}

qtHaveModule(gui):qtHaveModule(qml) {
    SUBDIRS += \
        declarative-radio \
        video
}

config_openal: SUBDIRS += audioengine

