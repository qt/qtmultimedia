load(qt_module)

TARGET = wmfengine
QT += multimediakit-private network multimediakitwidgets-private
PLUGIN_TYPE=mediaservice

load(qt_plugin)

DESTDIR = $$QT.multimediakit.plugins/$${PLUGIN_TYPE}

DEPENDPATH += .

HEADERS += wmfserviceplugin.h
SOURCES += wmfserviceplugin.cpp

include (player/player.pri)
