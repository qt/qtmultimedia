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
}

win32: {
    qtConfig(wmf): SUBDIRS += wmf
}

qtConfig(gstreamer): SUBDIRS += gstreamer

darwin:!watchos {
    qtConfig(avfoundation): SUBDIRS += avfoundation
}


