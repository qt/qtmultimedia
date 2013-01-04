TEMPLATE = subdirs

# These examples all need widgets for now (using creator templates that use widgets)
!isEmpty(QT.widgets.name) {
    SUBDIRS += \
        camera \
        videographicsitem \
        videowidget \
        player \
        customvideosurface
}

!isEmpty(QT.gui.name):!isEmpty(QT.qml.name) {
    disabled:SUBDIRS += declarative-camera
}

