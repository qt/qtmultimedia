QMAKE_USE_PRIVATE += alsa

HEADERS += platform/alsa/qalsaaudiodeviceinfo_p.h \
           platform/alsa/qalsaaudioinput_p.h \
           platform/alsa/qalsaaudiooutput_p.h \
           platform/alsa/qalsainterface_p.h

SOURCES += platform/alsa/qalsaaudiodeviceinfo.cpp \
           platform/alsa/qalsaaudioinput.cpp \
           platform/alsa/qalsaaudiooutput.cpp \
           platform/alsa/qalsainterface.cpp
