load(qt_module)

TARGET = qtmedia_audioengine
QT += multimedia-private
PLUGIN_TYPE=mediaservice

load(qt_plugin)
DESTDIR = $$QT.multimedia.plugins/$${PLUGIN_TYPE}

HEADERS += audioencodercontrol.h \
    audiocontainercontrol.h \
    audiomediarecordercontrol.h \
    audioendpointselector.h \
    audiocaptureservice.h \
    audiocaptureserviceplugin.h \
    audiocapturesession.h

SOURCES += audioencodercontrol.cpp \
    audiocontainercontrol.cpp \
    audiomediarecordercontrol.cpp \
    audioendpointselector.cpp \
    audiocaptureservice.cpp \
    audiocaptureserviceplugin.cpp \
    audiocapturesession.cpp

target.path += $$[QT_INSTALL_PLUGINS]/$${PLUGIN_TYPE}
INSTALLS += target

OTHER_FILES += \
    audiocapture.json
