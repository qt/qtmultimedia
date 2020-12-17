TARGET = gstaudiodecoder

include(../common.pri)

INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/qgstreameraudiodecoderservice.h \
    $$PWD/qgstreameraudiodecodercontrol.h \
    $$PWD/qgstreameraudiodecoderserviceplugin.h

SOURCES += \
    $$PWD/qgstreameraudiodecoderservice.cpp \
    $$PWD/qgstreameraudiodecodercontrol.cpp \
    $$PWD/qgstreameraudiodecoderserviceplugin.cpp

OTHER_FILES += \
    audiodecoder.json

PLUGIN_TYPE = mediaservice
PLUGIN_CLASS_NAME = QGstreamerAudioDecoderServicePlugin
load(qt_plugin)
