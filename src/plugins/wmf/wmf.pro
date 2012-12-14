TARGET = wmfengine
QT += multimedia-private network
!isEmpty(QT.widgets.name) {
    QT += multimediawidgets-private
    DEFINES += HAVE_WIDGETS
}

PLUGIN_TYPE=mediaservice
PLUGIN_CLASS_NAME = WMFServicePlugin
load(qt_plugin)

INCLUDEPATH += .

HEADERS += \
    wmfserviceplugin.h \
    mfstream.h \
    sourceresolver.h \
    samplegrabber.h \
    mftvideo.h

SOURCES += \
    wmfserviceplugin.cpp \
    mfstream.cpp \
    sourceresolver.cpp \
    samplegrabber.cpp \
    mftvideo.cpp

include (player/player.pri)
include (decoder/decoder.pri)

OTHER_FILES += \
    wmf.json
