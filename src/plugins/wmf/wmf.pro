TARGET = wmfengine
QT += multimedia-private network
qtHaveModule(widgets) {
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
    mftvideo.h \
    mfglobal.h \
    mfactivate.h

SOURCES += \
    wmfserviceplugin.cpp \
    mfstream.cpp \
    sourceresolver.cpp \
    samplegrabber.cpp \
    mftvideo.cpp \
    mfactivate.cpp \
    mfglobal.cpp

contains(QT_CONFIG, angle) {
    LIBS += -ld3d9 -ldxva2 -lwinmm -levr
    QT += gui-private

    HEADERS += \
        evrcustompresenter.h \
        evrd3dpresentengine.h

    SOURCES += \
        evrcustompresenter.cpp \
        evrd3dpresentengine.cpp
}


include (player/player.pri)
include (decoder/decoder.pri)

OTHER_FILES += \
    wmf.json
