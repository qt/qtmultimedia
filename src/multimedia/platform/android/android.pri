QT += opengl core-private network

HEADERS += \
    $$PWD/qandroidmediaserviceplugin_p.h

SOURCES += \
    $$PWD/qandroidmediaserviceplugin.cpp

include(audio/audio.pri)
include(wrappers/jni/jni.pri)
include(common/common.pri)
include(mediaplayer/mediaplayer.pri)
include(mediacapture/mediacapture.pri)
