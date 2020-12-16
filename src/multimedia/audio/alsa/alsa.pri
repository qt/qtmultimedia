QMAKE_USE_PRIVATE += alsa

HEADERS += audio/alsa/qalsaaudiodeviceinfo_p.h \
           audio/alsa/qalsaaudioinput_p.h \
           audio/alsa/qalsaaudiooutput_p.h \
           audio/alsa/qalsainterface_p.h

SOURCES += audio/alsa/qalsaaudiodeviceinfo.cpp \
           audio/alsa/qalsaaudioinput.cpp \
           audio/alsa/qalsaaudiooutput.cpp \
           audio/alsa/qalsainterface.cpp
