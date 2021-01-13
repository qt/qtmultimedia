LIBS += -lasound

HEADERS += \
           $$PWD/qnxaudiodeviceinfo_p.h \
           $$PWD/qnxaudioinput_p.h \
           $$PWD/qnxaudiooutput_p.h \
           $$PWD/qnxaudioutils_p.h \
           $$PWD/neutrinoserviceplugin_p.h

SOURCES += \
           $$PWD/qnxaudiodeviceinfo.cpp \
           $$PWD/qnxaudioinput.cpp \
           $$PWD/qnxaudiooutput.cpp \
           $$PWD/qnxaudioutils.cpp \
           $$PWD/neutrinoserviceplugin.cpp

include(common/common.pri)
include(mediaplayer/mediaplayer.pri)
include(camera/camera.pri)
