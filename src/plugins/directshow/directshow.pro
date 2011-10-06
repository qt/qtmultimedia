TEMPLATE = lib

CONFIG += plugin
TARGET = $$qtLibraryTarget(dsengine)

PLUGIN_TYPE=mediaservice

include (../../../common.pri)

QT += multimedia

DEPENDPATH += .

HEADERS += dsserviceplugin.h
SOURCES += dsserviceplugin.cpp

!contains(config_test_wmsdk, yes): DEFINES += QT_NO_WMSDK

contains(config_test_wmf, no): include (player/player.pri)
include (camera/camera.pri)

target.path += $$[QT_INSTALL_PLUGINS]/$${PLUGIN_TYPE}
INSTALLS += target
