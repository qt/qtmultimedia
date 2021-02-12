HEADERS += \
    $$PWD/qplatformmediacaptureinterface_p.h \
    $$PWD/qplatformmediaintegration_p.h \
    $$PWD/qplatformmediadevicemanager_p.h \
    $$PWD/qplatformmediaformatinfo_p.h

SOURCES += \
    $$PWD/qplatformmediacaptureinterface.cpp \
    $$PWD/qplatformmediaintegration.cpp \
    $$PWD/qplatformmediadevicemanager.cpp \
    $$PWD/qplatformmediaformatinfo.cpp

qtConfig(gstreamer):include(gstreamer/gstreamer.pri)
qtConfig(pulseaudio): include(pulseaudio/pulseaudio.pri)
qtConfig(alsa): include(alsa/alsa.pri)
android:include(android/android.pri)
win32:include(windows/windows.pri)
darwin:!watchos:include(darwin/darwin.pri)
qnx: include(qnx/qnx.pri)

