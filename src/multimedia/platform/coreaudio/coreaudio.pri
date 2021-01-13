HEADERS += \
    $$PWD/qcoreaudiodeviceinfo_p.h \
    $$PWD/qcoreaudioinput_p.h \
    $$PWD/qcoreaudiooutput_p.h \
    $$PWD/qcoreaudioutils_p.h

SOURCES += \
    $$PWD/qcoreaudiodeviceinfo.mm \
    $$PWD/qcoreaudioinput.mm \
    $$PWD/qcoreaudiooutput.mm \
    $$PWD/qcoreaudioutils.mm

ios|tvos {
    HEADERS += $$PWD/qcoreaudiosessionmanager_p.h
    SOURCES += $$PWD/qcoreaudiosessionmanager.mm
    LIBS += -framework Foundation -framework AVFoundation
} else {
    LIBS += \
        -framework ApplicationServices \
        -framework AudioUnit
}

LIBS += \
    -framework CoreAudio \
    -framework AudioToolbox
