TARGET = qtmedia_blackberry
QT += multimedia-private gui-private

PLUGIN_TYPE=mediaservice
PLUGIN_CLASS_NAME = BbServicePlugin
load(qt_plugin)

LIBS += -lmmrndclient -lstrm -lscreen

HEADERS += \
    bbserviceplugin.h \
    bbmediaplayerservice.h \
    bbmediaplayercontrol.h \
    bbmetadata.h \
    bbutil.h \
    bbvideowindowcontrol.h

SOURCES += \
    bbserviceplugin.cpp \
    bbmediaplayerservice.cpp \
    bbmediaplayercontrol.cpp \
    bbmetadata.cpp \
    bbutil.cpp \
    bbvideowindowcontrol.cpp

OTHER_FILES += blackberry_mediaservice.json
