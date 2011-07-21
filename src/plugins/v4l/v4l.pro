load(qt_module)

TARGET = qtmedia_v4lengine
QT += multimediakit-private
PLUGIN_TYPE = mediaservice

load(qt_plugin)
DESTDIR = $$QT.multimediakit.plugins/$${PLUGIN_TYPE}

HEADERS += v4lserviceplugin.h
SOURCES += v4lserviceplugin.cpp

include(radio/radio.pri)

target.path += $$[QT_INSTALL_PLUGINS]/$${PLUGIN_TYPE}
INSTALLS += target
