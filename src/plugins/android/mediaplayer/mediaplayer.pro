TARGET = androidmediaplayer
QT += multimedia-private gui-private platformsupport-private network

PLUGIN_TYPE = mediaservice
PLUGIN_CLASS_NAME = QAndroidMediaPlayerServicePlugin
load(qt_plugin)

HEADERS += \
    qandroidmediaplayercontrol.h \
    qandroidmediaservice.h \
    qandroidmetadatareadercontrol.h \
    qandroidmediaserviceplugin.h \
    qandroidvideorendercontrol.h \
    qandroidvideooutput.h
SOURCES += \
    qandroidmediaplayercontrol.cpp \
    qandroidmediaservice.cpp \
    qandroidmetadatareadercontrol.cpp \
    qandroidmediaserviceplugin.cpp \
    qandroidvideorendercontrol.cpp

OTHER_FILES += mediaplayer.json

include (../wrappers/wrappers.pri)
