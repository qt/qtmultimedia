TEMPLATE = subdirs

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
    player \

contains(QT_CONFIG, declarative) {
    disabled:SUBDIRS += declarative-camera
    SUBDIRS += declarative-radio
}

QT+=widgets
