TEMPLATE = subdirs

SUBDIRS += audiodecoder

# These examples all need widgets for now (using creator templates that use widgets)
!isEmpty(QT.widgets.name) {
    SUBDIRS += \
        radio \
        camera \
        spectrum \
        audiorecorder \
        audiodevices \
        audioinput \
        audiooutput \
        videographicsitem \
        videowidget \
        player \
        customvideosurface

    QT += widgets
}

!isEmpty(QT.gui.name):!isEmpty(QT.qml.name) {
    disabled:SUBDIRS += declarative-camera
    SUBDIRS += \
        declarative-radio \
        video
}

config_openal: SUBDIRS += audioengine

