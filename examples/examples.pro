TEMPLATE = subdirs

# These examples all need widgets for now (using creator templates that use widgets)
contains(config_test_widgets, yes) {
    SUBDIRS += \
        radio \
        camera \
        slideshow \
        spectrum \
        audiorecorder \
        audiodevices \
        audioinput \
        audiooutput \
        videographicsitem \
        videowidget \
        player

    contains(QT_CONFIG, declarative) {
        disabled:SUBDIRS += declarative-camera
        SUBDIRS += \
            declarative-radio \
            video
    }

    QT += widgets
}

