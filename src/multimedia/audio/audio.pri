INCLUDEPATH += audio

PUBLIC_HEADERS += \
           audio/qaudio.h \
           audio/qaudiobuffer.h \
           audio/qaudioformat.h \
           audio/qaudioinput.h \
           audio/qaudiooutput.h \
           audio/qaudiodeviceinfo.h \
           audio/qsoundeffect.h \
           audio/qaudioprobe.h \
           audio/qaudiodecoder.h

PRIVATE_HEADERS += \
           audio/qaudiobuffer_p.h \
           audio/qaudiodevicefactory_p.h \
           audio/qwavedecoder_p.h \
           audio/qsamplecache_p.h \
           audio/qaudiohelpers_p.h \
           audio/qaudiosystem_p.h  \
           audio/qsoundeffect_qaudio_p.h

SOURCES += \
           audio/qaudio.cpp \
           audio/qaudioformat.cpp  \
           audio/qaudiodeviceinfo.cpp \
           audio/qaudiooutput.cpp \
           audio/qaudioinput.cpp \
           audio/qaudiosystem.cpp \
           audio/qaudiodevicefactory.cpp \
           audio/qsoundeffect.cpp \
           audio/qwavedecoder_p.cpp \
           audio/qsamplecache_p.cpp \
           audio/qaudiobuffer.cpp \
           audio/qaudioprobe.cpp \
           audio/qaudiodecoder.cpp \
           audio/qaudiohelpers.cpp \
           audio/qsoundeffect_qaudio_p.cpp

android: include(opensles/opensles.pri)
win32: include(windows/windows.pri)
darwin:!watchos: include(coreaudio/coreaudio.pri)
qnx: include(qnx/qnx.pri)
qtConfig(pulseaudio): include(pulseaudio/pulseaudio.pri)
qtConfig(alsa): include(alsa/alsa.pri)

