HEADERS += \
    $$PWD/qmediaplatformintegration_p.h \
    $$PWD/qmediaplatformdevicemanager_p.h

SOURCES += \
    $$PWD/qmediaplatformintegration.cpp \
    $$PWD/qmediaplatformdevicemanager.cpp

qtConfig(gstreamer):include(gstreamer/gstreamer.pri)
qtConfig(pulseaudio): include(pulseaudio/pulseaudio.pri)
qtConfig(alsa): include(alsa/alsa.pri)

android {
    include(android/android.pri)
    include(opensles/opensles.pri)
}
win32 {
    include(wasapi/wasapi.pri)
    include(wmf/wmf.pri)
}
darwin:!watchos {
    include(coreaudio/coreaudio.pri)
    include(avfoundation/avfoundation.pri)
}
qnx: include(qnx/qnx.pri)

