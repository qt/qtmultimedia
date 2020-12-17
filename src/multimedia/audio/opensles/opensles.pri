LIBS += -lOpenSLES

HEADERS += \
    audio/opensles/qopenslesinterface_p.h \
    audio/opensles/qopenslesengine_p.h \
    audio/opensles/qopenslesdeviceinfo_p.h \
    audio/opensles/qopenslesaudioinput_p.h \
    audio/opensles/qopenslesaudiooutput_p.h

SOURCES += \
    audio/opensles/qopenslesinterface.cpp \
    audio/opensles/qopenslesengine.cpp \
    audio/opensles/qopenslesdeviceinfo.cpp \
    audio/opensles/qopenslesaudioinput.cpp \
    audio/opensles/qopenslesaudiooutput.cpp
