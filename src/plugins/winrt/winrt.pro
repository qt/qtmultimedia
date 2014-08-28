TARGET = winrtengine
QT += multimedia-private

PLUGIN_TYPE=mediaservice
PLUGIN_CLASS_NAME = WinRTServicePlugin
load(qt_plugin)

LIBS += -lmfplat -lmfuuid -loleaut32 -ld3d11

HEADERS += \
    qwinrtabstractvideorenderercontrol.h \
    qwinrtmediaplayercontrol.h \
    qwinrtmediaplayerservice.h \
    qwinrtplayerrenderercontrol.h \
    qwinrtserviceplugin.h

SOURCES += \
    qwinrtabstractvideorenderercontrol.cpp \
    qwinrtmediaplayercontrol.cpp \
    qwinrtmediaplayerservice.cpp \
    qwinrtplayerrenderercontrol.cpp \
    qwinrtserviceplugin.cpp

OTHER_FILES += \
    winrt.json
