load(qt_build_config)

# Avoid clash with a variable named `slots' in a Quartz header
CONFIG += no_keywords

TARGET = qavfcamera
QT += multimedia-private network

PLUGIN_TYPE = mediaservice

load(qt_plugin)
DESTDIR = $$QT.multimedia.plugins/$${PLUGIN_TYPE}

LIBS += -framework AppKit -framework AudioUnit \
        -framework AudioToolbox -framework CoreAudio \
        -framework QuartzCore -framework AVFoundation \
        -framework CoreMedia

target.path += $$[QT_INSTALL_PLUGINS]/$${PLUGIN_TYPE}
INSTALLS += target

OTHER_FILES += avfcamera.json

DEFINES += QMEDIA_AVF_CAMERA

HEADERS += \
    avfcameradebug.h \
    avfcameraserviceplugin.h \
    avfcameracontrol.h \
    avfvideorenderercontrol.h \
    avfcamerametadatacontrol.h \
    avfimagecapturecontrol.h \
    avfmediarecordercontrol.h \
    avfcameraservice.h \
    avfcamerasession.h \
    avfstoragelocation.h \
    avfvideodevicecontrol.h \
    avfaudioinputselectorcontrol.h \

OBJECTIVE_SOURCES += \
    avfcameraserviceplugin.mm \
    avfcameracontrol.mm \
    avfvideorenderercontrol.mm \
    avfcamerametadatacontrol.mm \
    avfimagecapturecontrol.mm \
    avfmediarecordercontrol.mm \
    avfcameraservice.mm \
    avfcamerasession.mm \
    avfstoragelocation.mm \
    avfvideodevicecontrol.mm \
    avfaudioinputselectorcontrol.mm \

