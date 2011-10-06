load(qt_module)

TARGET = wmfengine
QT += multimedia-private network multimediawidgets-private
PLUGIN_TYPE=mediaservice

load(qt_plugin)

DESTDIR = $$QT.multimedia.plugins/$${PLUGIN_TYPE}

DEPENDPATH += .

HEADERS += wmfserviceplugin.h
SOURCES += wmfserviceplugin.cpp

include (player/player.pri)
