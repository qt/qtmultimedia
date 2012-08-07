load(qt_build_config)

TARGET = QtMultimedia
QT = core-private network gui

contains(QT_CONFIG, opengl) | contains(QT_CONFIG, opengles2) {
} else {
   DEFINES += QT_NO_OPENGL
}

load(qt_module)

INCLUDEPATH *= .

PRIVATE_HEADERS += \
    qmediacontrol_p.h \
    qmediaobject_p.h \
    qmediapluginloader_p.h \
    qmediaservice_p.h \
    qmediaserviceprovider_p.h \
    qmediaresourcepolicyplugin_p.h \
    qmediaresourcepolicy_p.h \
    qmediaresourceset_p.h

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
    qtmedianamespace.cpp \
    qmediaresourcepolicyplugin_p.cpp \
    qmediaresourcepolicy_p.cpp \
    qmediaresourceset_p.cpp

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
win32:LIBS += -luuid

HEADERS += $$PUBLIC_HEADERS $$PRIVATE_HEADERS
