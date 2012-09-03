TARGET = gstaudiodecoder
PLUGIN_TYPE = mediaservice

load(qt_plugin)
DESTDIR = $$QT.multimedia.plugins/$${PLUGIN_TYPE}

include(../common.pri)

INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/qgstreameraudiodecodercontrol.h \
    $$PWD/qgstreameraudiodecoderservice.h \
    $$PWD/qgstreameraudiodecodersession.h \
    $$PWD/qgstreameraudiodecoderserviceplugin.h

SOURCES += \
    $$PWD/qgstreameraudiodecodercontrol.cpp \
    $$PWD/qgstreameraudiodecoderservice.cpp \
    $$PWD/qgstreameraudiodecodersession.cpp \
    $$PWD/qgstreameraudiodecoderserviceplugin.cpp

target.path += $$[QT_INSTALL_PLUGINS]/$${PLUGIN_TYPE}
INSTALLS += target

OTHER_FILES += \
    audiodecoder.json

