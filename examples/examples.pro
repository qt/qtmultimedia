TEMPLATE = subdirs

SUBDIRS += \
    radio \
    camera \
    slideshow \
    audiorecorder \
    audiodevices \
    audioinput \
    audiooutput \
    videographicsitem \
    videowidget

contains(QT_CONFIG, declarative) {
    SUBDIRS += declarative-camera
}

sources.path = $$QT_MOBILITY_EXAMPLES

INSTALLS += sources

