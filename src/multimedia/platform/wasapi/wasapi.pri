
LIBS += -lstrmiids -lole32 -loleaut32 -lwinmm

HEADERS += \
    platform/wasapi/qwindowsaudiointerface_p.h \
    platform/wasapi/qwindowsaudiodeviceinfo_p.h \
    platform/wasapi/qwindowsaudioinput_p.h \
    platform/wasapi/qwindowsaudiooutput_p.h \
    platform/wasapi/qwindowsaudioutils_p.h

SOURCES += \
    platform/wasapi/qwindowsaudiointerface.cpp \
    platform/wasapi/qwindowsaudiodeviceinfo.cpp \
    platform/wasapi/qwindowsaudioinput.cpp \
    platform/wasapi/qwindowsaudiooutput.cpp \
    platform/wasapi/qwindowsaudioutils.cpp

