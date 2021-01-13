QMAKE_USE_PRIVATE += pulseaudio

HEADERS += \
           $$PWD/qaudiodeviceinfo_pulse_p.h \
           $$PWD/qaudiooutput_pulse_p.h \
           $$PWD/qaudioinput_pulse_p.h \
           $$PWD/qaudioengine_pulse_p.h \
           $$PWD/qpulsehelpers_p.h \
           $$PWD/qpulseaudiodevicemanager_p.h \
           $$PWD/qpulseaudiointegration_p.h

SOURCES += \
           $$PWD/qaudiodeviceinfo_pulse.cpp \
           $$PWD/qaudiooutput_pulse.cpp \
           $$PWD/qaudioinput_pulse.cpp \
           $$PWD/qaudioengine_pulse.cpp \
           $$PWD/qpulsehelpers.cpp \
           $$PWD/qpulseaudiodevicemanager.cpp \
           $$PWD/qpulseaudiointegration.cpp

