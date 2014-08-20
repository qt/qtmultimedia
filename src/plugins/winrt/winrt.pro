TARGET = winrtengine
QT += multimedia-private

PLUGIN_TYPE=mediaservice
PLUGIN_CLASS_NAME = WinRTServicePlugin
load(qt_plugin)

LIBS += -lmfplat -lmfuuid -loleaut32 -ld3d11

HEADERS += \
    qwinrtserviceplugin.h

SOURCES += \
    qwinrtserviceplugin.cpp

OTHER_FILES += \
    winrt.json
