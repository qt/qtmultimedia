HEADERS += \
    $$PWD/qmediaplatformcaptureinterface_p.h \
    $$PWD/qmediaplatformintegration_p.h \
    $$PWD/qmediaplatformdevicemanager_p.h \
    $$PWD/qmediaplatformplayerinterface_p.h \
    $$PWD/qmediaplatformformatinfo_p.h

SOURCES += \
    $$PWD/qmediaplatformcaptureinterface.cpp \
    $$PWD/qmediaplatformintegration.cpp \
    $$PWD/qmediaplatformdevicemanager.cpp \
    $$PWD/qmediaplatformplayerinterface.cpp \
    $$PWD/qmediaplatformformatinfo.cpp

qtConfig(gstreamer):include(gstreamer/gstreamer.pri)
qtConfig(pulseaudio): include(pulseaudio/pulseaudio.pri)
qtConfig(alsa): include(alsa/alsa.pri)
android:include(android/android.pri)
win32:include(windows/windows.pri)
darwin:!watchos:include(darwin/darwin.pri)
qnx: include(qnx/qnx.pri)

