TEMPLATE = subdirs
QT_FOR_CONFIG += multimedia-private
CONFIG += add_ios_ffmpeg_libraries

# These examples all need widgets for now (using creator templates that use widgets)
qtHaveModule(widgets) {
    SUBDIRS += \
        audiodevices \
        audiooutput \
        audiorecorder \
        camera \
        player \
        videographicsitem \
        videowidget \
        screencapture
}

qtHaveModule(quick) {
    SUBDIRS += \
        declarative-camera \
        video
}
