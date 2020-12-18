######################################################################
#
# Qt Multimedia
#
######################################################################

TEMPLATE = subdirs
QT_FOR_CONFIG += multimedia-private

qtHaveModule(quick) {
   SUBDIRS += videonode
}

android {
   SUBDIRS += android
}

qnx {
    qtConfig(mmrenderer): SUBDIRS += qnx
    SUBDIRS += audiocapture
}

win32: {
    SUBDIRS += audiocapture

    qtConfig(wmf): SUBDIRS += wmf
}

qtConfig(gstreamer): SUBDIRS += gstreamer

unix:!mac:!android {
    !qtConfig(gstreamer): SUBDIRS += audiocapture
}

darwin:!watchos {
    SUBDIRS += audiocapture
    qtConfig(avfoundation): SUBDIRS += avfoundation
}


