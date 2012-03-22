load(qt_module)

TARGET = gstcamerabin
PLUGIN_TYPE = mediaservice

load(qt_plugin)
DESTDIR = $$QT.multimedia.plugins/$${PLUGIN_TYPE}

include(../common.pri)

INCLUDEPATH += $$PWD \
    $${SOURCE_DIR}/src/multimedia

INCLUDEPATH += camerabin

LIBS += -lgstphotography-0.10

DEFINES += GST_USE_UNSTABLE_API #prevents warnings because of unstable photography API

HEADERS += \
    $$PWD/camerabinservice.h \
    $$PWD/camerabinsession.h \
    $$PWD/camerabincontrol.h \
    $$PWD/camerabinaudioencoder.h \
    $$PWD/camerabinfocus.h \
    $$PWD/camerabinimageencoder.h \
    $$PWD/camerabinlocks.h \
    $$PWD/camerabinrecorder.h \
    $$PWD/camerabincontainer.h \
    $$PWD/camerabinexposure.h \
    $$PWD/camerabinflash.h \
    $$PWD/camerabinimagecapture.h \
    $$PWD/camerabinimageprocessing.h \
    $$PWD/camerabinmetadata.h \
    $$PWD/camerabinvideoencoder.h \
    $$PWD/camerabinresourcepolicy.h \
    $$PWD/camerabincapturedestination.h \
    $$PWD/camerabincapturebufferformat.h

SOURCES += \
    $$PWD/camerabinservice.cpp \
    $$PWD/camerabinsession.cpp \
    $$PWD/camerabincontrol.cpp \
    $$PWD/camerabinaudioencoder.cpp \
    $$PWD/camerabincontainer.cpp \
    $$PWD/camerabinexposure.cpp \
    $$PWD/camerabinflash.cpp \
    $$PWD/camerabinfocus.cpp \
    $$PWD/camerabinimagecapture.cpp \
    $$PWD/camerabinimageencoder.cpp \
    $$PWD/camerabinimageprocessing.cpp \
    $$PWD/camerabinlocks.cpp \
    $$PWD/camerabinmetadata.cpp \
    $$PWD/camerabinrecorder.cpp \
    $$PWD/camerabinvideoencoder.cpp \
    $$PWD/camerabinresourcepolicy.cpp \
    $$PWD/camerabincapturedestination.cpp \
    $$PWD/camerabincapturebufferformat.cpp

maemo6 {
    HEADERS += \
        $$PWD/camerabuttonlistener_meego.h

    SOURCES += \
        $$PWD/camerabuttonlistener_meego.cpp
}

target.path += $$[QT_INSTALL_PLUGINS]/$${PLUGIN_TYPE}
INSTALLS += target

