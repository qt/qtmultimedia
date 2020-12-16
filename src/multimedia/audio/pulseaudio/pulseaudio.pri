QMAKE_USE_PRIVATE += pulseaudio

HEADERS += audio/pulseaudio/qaudiointerface_pulse_p.h \
           audio/pulseaudio/qaudiodeviceinfo_pulse_p.h \
           audio/pulseaudio/qaudiooutput_pulse_p.h \
           audio/pulseaudio/qaudioinput_pulse_p.h \
           audio/pulseaudio/qaudioengine_pulse_p.h \
           audio/pulseaudio/qpulsehelpers_p.h

SOURCES += audio/pulseaudio/qaudiointerface_pulse.cpp \
           audio/pulseaudio/qaudiodeviceinfo_pulse.cpp \
           audio/pulseaudio/qaudiooutput_pulse.cpp \
           audio/pulseaudio/qaudioinput_pulse.cpp \
           audio/pulseaudio/qaudioengine_pulse.cpp \
           audio/pulseaudio/qpulsehelpers.cpp
