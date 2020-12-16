HEADERS += \
    audio/coreaudio/qcoreaudiodeviceinfo_p.h \
    audio/coreaudio/qcoreaudioinput_p.h \
    audio/coreaudio/qcoreaudiooutput_p.h \
    audio/coreaudio/qcoreaudiointerface_p.h \
    audio/coreaudio/qcoreaudioutils_p.h

OBJECTIVE_SOURCES += \
    audio/coreaudio/qcoreaudiodeviceinfo.mm \
    audio/coreaudio/qcoreaudioinput.mm \
    audio/coreaudio/qcoreaudiooutput.mm \
    audio/coreaudio/qcoreaudiointerface.mm \
    audio/coreaudio/qcoreaudioutils.mm

ios|tvos {
    HEADERS += audio/coreaudio/qcoreaudiosessionmanager_p.h
    OBJECTIVE_SOURCES += audio/coreaudio/qcoreaudiosessionmanager.mm
    LIBS += -framework Foundation -framework AVFoundation
} else {
    LIBS += \
        -framework ApplicationServices \
        -framework AudioUnit
}

LIBS += \
    -framework CoreAudio \
    -framework AudioToolbox
