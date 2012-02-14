INCLUDEPATH += $$PWD

DEFINES += QMEDIA_GSTREAMER_AUDIO_DECODER

HEADERS += \
    $$PWD/qgstreameraudiodecodercontrol.h \
    $$PWD/qgstreameraudiodecoderservice.h \
    $$PWD/qgstreameraudiodecodersession.h

SOURCES += \
    $$PWD/qgstreameraudiodecodercontrol.cpp \
    $$PWD/qgstreameraudiodecoderservice.cpp \
    $$PWD/qgstreameraudiodecodersession.cpp


