TARGET = qtmedia_qnx_audio
QT += multimedia-private
CONFIG += no_private_qt_headers_warning

PLUGIN_TYPE = audio

load(qt_plugin)
DESTDIR = $$QT.multimedia.plugins/$${PLUGIN_TYPE}
LIBS += -lasound

HEADERS += qnxaudioplugin.h \
           qnxaudiodeviceinfo.h \
           qnxaudiooutput.h \
           qnxaudioutils.h

SOURCES += qnxaudioplugin.cpp \
           qnxaudiodeviceinfo.cpp \
           qnxaudiooutput.cpp \
           qnxaudioutils.cpp

OTHER_FILES += qnx_audio.json

target.path += $$[QT_INSTALL_PLUGINS]/$${PLUGIN_TYPE}
INSTALLS += target
