TEMPLATE = subdirs

# These examples all need widgets for now (using creator templates that use widgets)
qtHaveModule(widgets) {
    SUBDIRS += \
        camera \
        videographicsitem \
        videowidget \
        player \
        customvideosurface
}

qtHaveModule(gui):qtHaveModule(qml) {
    disabled:SUBDIRS += declarative-camera
}

