TEMPLATE = lib

CONFIG += plugin
TARGET = $$qtLibraryTarget(dsengine)

PLUGIN_TYPE=mediaservice

QT += multimedia

HEADERS += dsserviceplugin.h
SOURCES += dsserviceplugin.cpp

!config_wmsdk: DEFINES += QT_NO_WMSDK

!isEmpty(QT.widgets.name) {
    QT += multimediawidgets
    DEFINES += HAVE_WIDGETS
}

win32-g++ {
    DEFINES += NO_DSHOW_STRSAFE
}

!config_wmf: include(player/player.pri)
include(camera/camera.pri)

target.path += $$[QT_INSTALL_PLUGINS]/$${PLUGIN_TYPE}
INSTALLS += target

OTHER_FILES += \
    directshow.json
