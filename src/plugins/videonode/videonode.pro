TEMPLATE = subdirs
QT_FOR_CONFIG += gui-private multimedia-private

qtConfig(gpu_vivante):qtConfig(gstreamer) {
    SUBDIRS += imx6
}

qtConfig(egl):qtConfig(opengles2):!android: SUBDIRS += egl
