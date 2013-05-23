TARGET = dsengine

PLUGIN_TYPE=mediaservice
PLUGIN_CLASS_NAME = DSServicePlugin
load(qt_plugin)

QT += multimedia

HEADERS += dsserviceplugin.h
SOURCES += dsserviceplugin.cpp

!config_wmsdk: DEFINES += QT_NO_WMSDK

qtHaveModule(widgets) {
    QT += multimediawidgets
    DEFINES += HAVE_WIDGETS
}

win32-g++ {
    DEFINES += NO_DSHOW_STRSAFE
}

!config_wmf: include(player/player.pri)
include(camera/camera.pri)

OTHER_FILES += \
    directshow.json \
    directshow_camera.json
