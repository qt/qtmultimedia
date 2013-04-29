TARGET = qtmedia_blackberry
QT += multimedia-private gui-private

PLUGIN_TYPE=mediaservice
PLUGIN_CLASS_NAME = BbServicePlugin
load(qt_plugin)

LIBS += -lscreen

HEADERS += bbserviceplugin.h
SOURCES += bbserviceplugin.cpp

include(common/common.pri)

!blackberry-playbook {
    include(camera/camera.pri)
}

include(mediaplayer/mediaplayer.pri)

OTHER_FILES += blackberry_mediaservice.json
