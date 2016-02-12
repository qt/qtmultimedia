TARGET = qtmedia_qnx_audio
QT += multimedia-private
CONFIG += no_private_qt_headers_warning


LIBS += -lasound

HEADERS += qnxaudioplugin.h \
           qnxaudiodeviceinfo.h \
           qnxaudioinput.h \
           qnxaudiooutput.h \
           qnxaudioutils.h

SOURCES += qnxaudioplugin.cpp \
           qnxaudiodeviceinfo.cpp \
           qnxaudioinput.cpp \
           qnxaudiooutput.cpp \
           qnxaudioutils.cpp

OTHER_FILES += qnx_audio.json

PLUGIN_TYPE = audio
load(qt_plugin)
