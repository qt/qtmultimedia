load(qt_module)

TARGET = qtmultimediakit_m3u
QT += multimediakit-private
PLUGIN_TYPE=playlistformats

load(qt_plugin)
DESTDIR = $$QT.multimediakit.plugins/$${PLUGIN_TYPE}


HEADERS += qm3uhandler.h
SOURCES += main.cpp \
           qm3uhandler.cpp
