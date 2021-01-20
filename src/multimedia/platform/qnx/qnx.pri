HEADERS += \
           $$PWD/qqnxdevicemanager_p.h \
           $$PWD/qqnxintegration_p.h

SOURCES += \
           $$PWD/qqnxdevicemanager.cpp \
           $$PWD/qqnxintegration.cpp

include(audio/audio.pri)
include(common/common.pri)
include(mediaplayer/mediaplayer.pri)
include(camera/camera.pri)
