TEMPLATE = subdirs
QT_FOR_CONFIG += gui-private

config_gpu_vivante {
    SUBDIRS += imx6
}

qtConfig(egl):qtConfig(opengles2):!android: SUBDIRS += egl
