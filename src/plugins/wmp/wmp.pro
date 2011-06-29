TEMPLATE = lib
CONFIG += plugin
TARGET = $$qtLibraryTarget(qtmedia_wmp)

PLUGIN_TYPE=mediaservice

INCLUDEPATH+=../../../src/multimedia
include(../../../common.pri)

CONFIG += mobility
MOBILITY = multimedia
LIBS += -lstrmiids -lole32 -lOleaut32 -luser32 -lgdi32

HEADERS = \
    qmfactivate.h \
    qwmpevents.h \
    qwmpglobal.h \
    qwmpmetadata.h \
    qwmpplayercontrol.h \
    qwmpplayerservice.h \
    qwmpplaylist.h \
    qwmpplaylistcontrol.h \
    qwmpserviceprovider.h \
    qwmpvideooverlay.h

SOURCES = \
    qmfactivate.cpp \
    qwmpevents.cpp \
    qwmpglobal.cpp \
    qwmpmetadata.cpp \
    qwmpplayercontrol.cpp \
    qwmpplayerservice.cpp \
    qwmpplaylist.cpp \
    qwmpplaylistcontrol.cpp \
    qwmpserviceprovider.cpp \
    qwmpvideooverlay.cpp

contains(evr_enabled, yes) {
    HEADERS += \
            qevrvideooverlay.h

    SOURCES += \
            qevrvideooverlay.cpp

    DEFINES += \
        QWMP_EVR
}
