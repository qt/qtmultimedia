INCLUDEPATH += $$PWD

DEFINES += AUDIOSOURCEUSED

symbian:LIBS += -lmediaclientaudio \
    -lmmfcontrollerframework \
    -lefsrv \
    -lbafl \

!contains(S60_VERSION, 3.1) {
    contains(audiorouting_s60_enabled,yes) {
        #We use audioinputrouting.lib for recording audio from different sources -lmediaclientaudioinputstream \ -lcone \
        DEFINES += AUDIOINPUT_ROUTING
        message("Audio Input Routing enabled onwards 3.2 SDK")
        LIBS += -laudioinputrouting
    }
}

HEADERS += $$PWD/s60audioencodercontrol.h \
    $$PWD/s60audiomediarecordercontrol.h \
    $$PWD/s60audioendpointselector.h \
    $$PWD/s60audiocaptureservice.h \
    $$PWD/s60audiocapturesession.h \
    $$PWD/S60audiocontainercontrol.h

SOURCES += $$PWD/s60audioencodercontrol.cpp \
    $$PWD/s60audiomediarecordercontrol.cpp \
    $$PWD/s60audioendpointselector.cpp \
    $$PWD/s60audiocaptureservice.cpp \
    $$PWD/s60audiocapturesession.cpp \
    $$PWD/S60audiocontainercontrol.cpp
