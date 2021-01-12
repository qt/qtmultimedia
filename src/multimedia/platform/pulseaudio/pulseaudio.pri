QMAKE_USE_PRIVATE += pulseaudio

HEADERS += $$PWD/qaudiointerface_pulse_p.h \
           $$PWD/qaudiodeviceinfo_pulse_p.h \
           $$PWD/qaudiooutput_pulse_p.h \
           $$PWD/qaudioinput_pulse_p.h \
           $$PWD/qaudioengine_pulse_p.h \
           $$PWD/qpuls_p.helpers_p.h

SOURCES += $$PWD/qaudiointerface_pulse.cpp \
           $$PWD/qaudiodeviceinfo_pulse.cpp \
           $$PWD/qaudiooutput_pulse.cpp \
           $$PWD/qaudioinput_pulse.cpp \
           $$PWD/qaudioengine_pulse.cpp \
           $$PWD/qpuls_p.helpers.cpp
