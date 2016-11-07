TARGET = qtmedia_qnx
QT += multimedia-private gui-private

LIBS += -lscreen

include(common/common.pri)
include(mediaplayer/mediaplayer.pri)

blackberry {
    include(camera/camera.pri)
    HEADERS += bbserviceplugin.h
    SOURCES += bbserviceplugin.cpp
    OTHER_FILES += blackberry_mediaservice.json
    PLUGIN_CLASS_NAME = BbServicePlugin
} else {
    HEADERS += neutrinoserviceplugin.h
    SOURCES += neutrinoserviceplugin.cpp
    OTHER_FILES += neutrino_mediaservice.json
    PLUGIN_CLASS_NAME = NeutrinoServicePlugin
}

PLUGIN_TYPE = mediaservice
load(qt_plugin)
