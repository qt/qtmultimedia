TEMPLATE = subdirs

SUBDIRS += audiodecoder

# These examples all need widgets for now (using creator templates that use widgets)
!isEmpty(QT.widgets.name) {
    SUBDIRS += \
        radio \
        spectrum \
        audiorecorder \
        audiodevices \
        audioinput \
        audiooutput \
}

!isEmpty(QT.gui.name):!isEmpty(QT.qml.name) {
    SUBDIRS += \
        declarative-radio \
        video
}

config_openal: SUBDIRS += audioengine

