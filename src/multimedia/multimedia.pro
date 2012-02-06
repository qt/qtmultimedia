load(qt_module)

TARGET = QtMultimedia
QPRO_PWD = $$PWD
QT = core network gui

CONFIG += module
MODULE_PRI += ../../modules/qt_multimedia.pri

contains(QT_CONFIG, opengl) | contains(QT_CONFIG, opengles2) {
} else {
   DEFINES += QT_NO_OPENGL
}

!static:DEFINES += QT_MAKEDLL
DEFINES += QT_BUILD_MULTIMEDIA_LIB

load(qt_module_config)

HEADERS += qtmultimediaversion.h

INCLUDEPATH *= .

PRIVATE_HEADERS += \
    qmediacontrol_p.h \
    qmediaobject_p.h \
    qmediapluginloader_p.h \
    qmediaservice_p.h \
    qmediaserviceprovider_p.h \

PUBLIC_HEADERS += \
    qmediabindableinterface.h \
    qmediacontrol.h \
    qmediaenumdebug.h \
    qmediaobject.h \
    qmediaservice.h \
    qmediaserviceproviderplugin.h \
    qmediatimerange.h \
    qtmedianamespace.h \
    qtmultimediadefs.h \

SOURCES += \
    qmediabindableinterface.cpp \
    qmediacontrol.cpp \
    qmediaobject.cpp \
    qmediapluginloader.cpp \
    qmediaservice.cpp \
    qmediaserviceprovider.cpp \
    qmediatimerange.cpp \
    qtmedianamespace.cpp

include(audio/audio.pri)
include(camera/camera.pri)
include(controls/controls.pri)
include(playback/playback.pri)
include(radio/radio.pri)
include(recording/recording.pri)
include(video/video.pri)

mac {
   LIBS += -framework AppKit -framework QuartzCore -framework QTKit
}

HEADERS += $$PUBLIC_HEADERS $$PRIVATE_HEADERS

OTHER_FILES += \
    qaudionamespace.qdoc \

