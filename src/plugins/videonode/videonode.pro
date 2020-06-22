TEMPLATE = subdirs
QT_FOR_CONFIG += gui-private multimedia-private

qtConfig(gpu_vivante) {
    SUBDIRS += imx6
}
