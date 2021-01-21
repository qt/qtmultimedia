QT += opengl core-private network

HEADERS += \
    $$PWD/qandroidintegration_p.h \
    $$PWD/qandroiddevicemanager_p.h

SOURCES += \
    $$PWD/qandroidintegration.cpp \
    $$PWD/qandroiddevicemanager.cpp

include(audio/audio.pri)
include(wrappers/jni/jni.pri)
include(common/common.pri)
include(mediaplayer/mediaplayer.pri)
include(mediacapture/mediacapture.pri)
