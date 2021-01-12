QMAKE_USE_PRIVATE += pulseaudio

HEADERS += platform/pulseaudio/qaudiointerface_pulse_p.h \
           platform/pulseaudio/qaudiodeviceinfo_pulse_p.h \
           platform/pulseaudio/qaudiooutput_pulse_p.h \
           platform/pulseaudio/qaudioinput_pulse_p.h \
           platform/pulseaudio/qaudioengine_pulse_p.h \
           platform/pulseaudio/qpulsehelpers_p.h

SOURCES += platform/pulseaudio/qaudiointerface_pulse.cpp \
           platform/pulseaudio/qaudiodeviceinfo_pulse.cpp \
           platform/pulseaudio/qaudiooutput_pulse.cpp \
           platform/pulseaudio/qaudioinput_pulse.cpp \
           platform/pulseaudio/qaudioengine_pulse.cpp \
           platform/pulseaudio/qpulsehelpers.cpp
