TARGET = qtmedia_qnx
QT += multimedia-private gui-private core-private

LIBS += -lscreen

include(common/common.pri)
include(mediaplayer/mediaplayer.pri)

HEADERS += neutrinoserviceplugin.h
SOURCES += neutrinoserviceplugin.cpp
OTHER_FILES += neutrino_mediaservice.json

PLUGIN_TYPE = mediaservice
PLUGIN_CLASS_NAME = BbServicePlugin
load(qt_plugin)
