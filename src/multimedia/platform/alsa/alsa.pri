QMAKE_USE_PRIVATE += alsa

HEADERS += $$PWD/qalsaaudiodeviceinfo_p.h \
           $$PWD/qalsaaudioinput_p.h \
           $$PWD/qalsaaudiooutput_p.h \
           $$PWD/qalsainterface_p.h

SOURCES += $$PWD/qalsaaudiodeviceinfo.cpp \
           $$PWD/qalsaaudioinput.cpp \
           $$PWD/qalsaaudiooutput.cpp \
           $$PWD/qalsainterface.cpp
