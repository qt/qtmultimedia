HEADERS += \
    $$PWD/qmediaplatformintegration_p.h \
    $$PWD/qmediaplatformdevicemanager_p.h

SOURCES += \
    $$PWD/qmediaplatformintegration.cpp \
    $$PWD/qmediaplatformdevicemanager.cpp

qtConfig(gstreamer):include(gstreamer/gstreamer.pri)
qtConfig(pulseaudio): include(pulseaudio/pulseaudio.pri)
qtConfig(alsa): include(alsa/alsa.pri)
android:include(android/android.pri)
win32:include(windows/windows.pri)
darwin:!watchos:include(darwin/darwin.pri)
qnx: include(qnx/qnx.pri)

