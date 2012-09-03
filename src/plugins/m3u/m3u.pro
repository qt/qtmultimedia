TARGET = qtmultimedia_m3u
QT += multimedia-private
PLUGIN_TYPE=playlistformats

load(qt_plugin)
DESTDIR = $$QT.multimedia.plugins/$${PLUGIN_TYPE}


HEADERS += qm3uhandler.h
SOURCES += qm3uhandler.cpp

target.path += $$[QT_INSTALL_PLUGINS]/$${PLUGIN_TYPE}
INSTALLS += target

OTHER_FILES += \
    m3u.json
