TEMPLATE = lib

CONFIG += plugin
TARGET = $$qtLibraryTarget(dsengine)

PLUGIN_TYPE=mediaservice

include (../../../common.pri)
INCLUDEPATH+=../../multimediakit \
             ../../multimediakit/audio \
             ../../multimediakit/video

qtAddLibrary(QtMultimediaKit)

DEPENDPATH += .

HEADERS += dsserviceplugin.h
SOURCES += dsserviceplugin.cpp

!contains(config_test_wmsdk, yes): DEFINES += QT_NO_WMSDK

include (player/player.pri)
include (camera/camera.pri)

target.path += $$[QT_INSTALL_PLUGINS]/$${PLUGIN_TYPE}
INSTALLS += target
