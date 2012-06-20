load(qt_module)

TARGET = wmfengine
QT += multimedia-private network
!isEmpty(QT.widgets.name) {
    QT += multimediawidgets-private
    DEFINES += HAVE_WIDGETS
}
PLUGIN_TYPE=mediaservice

load(qt_plugin)

DESTDIR = $$QT.multimedia.plugins/$${PLUGIN_TYPE}

DEPENDPATH += .

HEADERS += wmfserviceplugin.h
SOURCES += wmfserviceplugin.cpp

include (player/player.pri)

OTHER_FILES += \
    wmf.json
