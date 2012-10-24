TARGET = qtmultimedia_m3u
QT += multimedia-private

PLUGIN_TYPE=playlistformats
load(qt_plugin)

HEADERS += qm3uhandler.h
SOURCES += qm3uhandler.cpp

OTHER_FILES += \
    m3u.json
