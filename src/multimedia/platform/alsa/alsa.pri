QMAKE_USE_PRIVATE += alsa

HEADERS += $$PWD/qalsaaudiodeviceinfo_p.h \
           $$PWD/qalsaaudioinput_p.h \
           $$PWD/qalsaaudiooutput_p.h \
           $$PWD/qalsadevicemanager_p.h \
           $$PWD/qalsaintegration_p.h

SOURCES += $$PWD/qalsaaudiodeviceinfo.cpp \
           $$PWD/qalsaaudioinput.cpp \
           $$PWD/qalsaaudiooutput.cpp \
           $$PWD/qalsadevicemanager.cpp \
           $$PWD/qalsaintegration.cpp
