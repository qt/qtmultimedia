TARGET = dsengine
win32:!qtHaveModule(opengl)|contains(QT_CONFIG,dynamicgl) {
    LIBS_PRIVATE += -lgdi32 -luser32
}

QT += multimedia-private

HEADERS += dsserviceplugin.h
SOURCES += dsserviceplugin.cpp

!config_wmsdk: DEFINES += QT_NO_WMSDK

mingw: DEFINES += NO_DSHOW_STRSAFE

!config_wmf: include(player/player.pri)
!wince: include(camera/camera.pri)

OTHER_FILES += \
    directshow.json \
    directshow_camera.json

PLUGIN_TYPE = mediaservice
PLUGIN_CLASS_NAME = DSServicePlugin
load(qt_plugin)
