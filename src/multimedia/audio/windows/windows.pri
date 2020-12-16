
LIBS += -lstrmiids -lole32 -loleaut32 -lwinmm

HEADERS += \
    audio/windows/qwindowsaudiointerface_p.h \
    audio/windows/qwindowsaudiodeviceinfo_p.h \
    audio/windows/qwindowsaudioinput_p.h \
    audio/windows/qwindowsaudiooutput_p.h \
    audio/windows/qwindowsaudioutils_p.h

SOURCES += \
    audio/windows/qwindowsaudiointerface.cpp \
    audio/windows/qwindowsaudiodeviceinfo.cpp \
    audio/windows/qwindowsaudioinput.cpp \
    audio/windows/qwindowsaudiooutput.cpp \
    audio/windows/qwindowsaudioutils.cpp

