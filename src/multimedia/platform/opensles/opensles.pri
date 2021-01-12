LIBS += -lOpenSLES

HEADERS += \
    platform/opensles/qopenslesinterface_p.h \
    platform/opensles/qopenslesengine_p.h \
    platform/opensles/qopenslesdeviceinfo_p.h \
    platform/opensles/qopenslesaudioinput_p.h \
    platform/opensles/qopenslesaudiooutput_p.h

SOURCES += \
    platform/opensles/qopenslesinterface.cpp \
    platform/opensles/qopenslesengine.cpp \
    platform/opensles/qopenslesdeviceinfo.cpp \
    platform/opensles/qopenslesaudioinput.cpp \
    platform/opensles/qopenslesaudiooutput.cpp
