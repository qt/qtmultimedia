TARGET = gstcamerabin

QT += multimedia-private

include(../common.pri)

INCLUDEPATH += $$PWD \
    $${SOURCE_DIR}/src/multimedia

INCLUDEPATH += camerabin


HEADERS += \
    $$PWD/camerabinserviceplugin.h \
    $$PWD/camerabinservice.h \
    $$PWD/camerabinsession.h \
    $$PWD/camerabincontrol.h \
    $$PWD/camerabinaudioencoder.h \
    $$PWD/camerabinimageencoder.h \
    $$PWD/camerabinrecorder.h \
    $$PWD/camerabincontainer.h \
    $$PWD/camerabinimagecapture.h \
    $$PWD/camerabinimageprocessing.h \
    $$PWD/camerabinmetadata.h \
    $$PWD/camerabinvideoencoder.h \

SOURCES += \
    $$PWD/camerabinserviceplugin.cpp \
    $$PWD/camerabinservice.cpp \
    $$PWD/camerabinsession.cpp \
    $$PWD/camerabincontrol.cpp \
    $$PWD/camerabinaudioencoder.cpp \
    $$PWD/camerabincontainer.cpp \
    $$PWD/camerabinimagecapture.cpp \
    $$PWD/camerabinimageencoder.cpp \
    $$PWD/camerabinimageprocessing.cpp \
    $$PWD/camerabinmetadata.cpp \
    $$PWD/camerabinrecorder.cpp \
    $$PWD/camerabinvideoencoder.cpp \

qtConfig(gstreamer_photography) {
    HEADERS += \
        $$PWD/camerabinfocus.h \
        $$PWD/camerabinexposure.h \

    SOURCES += \
        $$PWD/camerabinexposure.cpp \
        $$PWD/camerabinfocus.cpp \

    QMAKE_USE += gstreamer_photography
    DEFINES += GST_USE_UNSTABLE_API #prevents warnings because of unstable photography API
}

qtConfig(linux_v4l) {
    HEADERS += \
        $$PWD/camerabinv4limageprocessing.h

    SOURCES += \
        $$PWD/camerabinv4limageprocessing.cpp
}

OTHER_FILES += \
    camerabin.json

PLUGIN_TYPE = mediaservice
PLUGIN_CLASS_NAME = CameraBinServicePlugin
load(qt_plugin)
