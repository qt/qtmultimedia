TARGET = gstmediaplayer
PLUGIN_TYPE = mediaservice

load(qt_plugin)
DESTDIR = $$QT.multimedia.plugins/$${PLUGIN_TYPE}

include(../common.pri)

INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/qgstreamerplayercontrol.h \
    $$PWD/qgstreamerplayerservice.h \
    $$PWD/qgstreamerplayersession.h \
    $$PWD/qgstreamerstreamscontrol.h \
    $$PWD/qgstreamermetadataprovider.h \
    $$PWD/qgstreameravailabilitycontrol.h \
    $$PWD/qgstreamerplayerserviceplugin.h

SOURCES += \
    $$PWD/qgstreamerplayercontrol.cpp \
    $$PWD/qgstreamerplayerservice.cpp \
    $$PWD/qgstreamerplayersession.cpp \
    $$PWD/qgstreamerstreamscontrol.cpp \
    $$PWD/qgstreamermetadataprovider.cpp \
    $$PWD/qgstreameravailabilitycontrol.cpp \
    $$PWD/qgstreamerplayerserviceplugin.cpp

target.path += $$[QT_INSTALL_PLUGINS]/$${PLUGIN_TYPE}
INSTALLS += target

OTHER_FILES += \
    mediaplayer.json

