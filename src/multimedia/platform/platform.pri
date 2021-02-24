HEADERS += \
    $$PWD/qplatformmediacapture_p.h \
    $$PWD/qplatformmediaintegration_p.h \
    $$PWD/qplatformmediadevicemanager_p.h \
    $$PWD/qplatformmediaformatinfo_p.h \
    $$PWD/qplatformaudiodecoder_p.h \
    $$PWD/platformcamera.h \
    $$PWD/qplatformcameraexposure_p.h \
    $$PWD/platform/qplatformcamerafocus_p.h \
    $$PWD/qplatformcameraimagecapture_p.h \
    $$PWD/qplatformcameraimageprocessing_p.h \
    $$PWD/qplatformmediarecorder_p.h \

SOURCES += \
    $$PWD/qplatformmediacapture.cpp \
    $$PWD/qplatformmediaintegration.cpp \
    $$PWD/qplatformmediadevicemanager.cpp \
    $$PWD/qplatformmediaformatinfo.cpp \
    $$PWD/qplatformaudiodecoder.cpp \
    $$PWD/platformcamera.cpp \
    $$PWD/qplatformcameraexposure.cpp \
    $$PWD/platform/qplatformcamerafocus.cpp \
    $$PWD/qplatformcameraimagecapture.cpp \
    $$PWD/qplatformcameraimageprocessing.cpp \
    $$PWD/qplatformmediarecorder.cpp \

qtConfig(gstreamer):include(gstreamer/gstreamer.pri)
qtConfig(pulseaudio): include(pulseaudio/pulseaudio.pri)
qtConfig(alsa): include(alsa/alsa.pri)
android:include(android/android.pri)
win32:include(windows/windows.pri)
darwin:!watchos:include(darwin/darwin.pri)
qnx: include(qnx/qnx.pri)

