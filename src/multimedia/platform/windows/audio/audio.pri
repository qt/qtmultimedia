
LIBS += -lstrmiids -lole32 -loleaut32 -lwinmm

HEADERS += \
    $$PWD/qwindowsaudiodeviceinfo_p.h \
    $$PWD/qwindowsaudioinput_p.h \
    $$PWD/qwindowsaudiooutput_p.h \
    $$PWD/qwindowsaudioutils_p.h

SOURCES += \
    $$PWD/qwindowsaudiodeviceinfo.cpp \
    $$PWD/qwindowsaudioinput.cpp \
    $$PWD/qwindowsaudiooutput.cpp \
    $$PWD/qwindowsaudioutils.cpp

