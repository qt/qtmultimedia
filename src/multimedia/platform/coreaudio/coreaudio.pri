HEADERS += \
    platform/coreaudio/qcoreaudiodeviceinfo_p.h \
    platform/coreaudio/qcoreaudioinput_p.h \
    platform/coreaudio/qcoreaudiooutput_p.h \
    platform/coreaudio/qcoreaudiointerface_p.h \
    platform/coreaudio/qcoreaudioutils_p.h

SOURCES += \
    platform/coreaudio/qcoreaudiodeviceinfo.mm \
    platform/coreaudio/qcoreaudioinput.mm \
    platform/coreaudio/qcoreaudiooutput.mm \
    platform/coreaudio/qcoreaudiointerface.mm \
    platform/coreaudio/qcoreaudioutils.mm

ios|tvos {
    HEADERS += platform/coreaudio/qcoreaudiosessionmanager_p.h
    SOURCES += platform/coreaudio/qcoreaudiosessionmanager.mm
    LIBS += -framework Foundation -framework AVFoundation
} else {
    LIBS += \
        -framework ApplicationServices \
        -framework AudioUnit
}

LIBS += \
    -framework CoreAudio \
    -framework AudioToolbox
