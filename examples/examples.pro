TEMPLATE = subdirs

# These examples all need widgets for now (using creator templates that use widgets)
!isEmpty(QT.widgets.name) {
    SUBDIRS += \
        radio \
        camera \
        spectrum \
        audiodecoder \
        audiorecorder \
        audiodevices \
        audioinput \
        audiooutput \
        videographicsitem \
        videowidget \
        player

    contains(QT_CONFIG, qml) {
        disabled:SUBDIRS += declarative-camera
        SUBDIRS += \
            declarative-radio \
            video
    }

    QT += widgets
}

config_openal: SUBDIRS += audioengine
