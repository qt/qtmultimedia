LIBS += -lOpenSLES

HEADERS += \
    $$PWD/qopenslesinterface_p.h \
    $$PWD/qopenslesengine_p.h \
    $$PWD/qopenslesdeviceinfo_p.h \
    $$PWD/qopenslesaudioinput_p.h \
    $$PWD/qopenslesaudiooutput_p.h

SOURCES += \
    $$PWD/qopenslesinterface.cpp \
    $$PWD/qopenslesengine.cpp \
    $$PWD/qopenslesdeviceinfo.cpp \
    $$PWD/qopenslesaudioinput.cpp \
    $$PWD/qopenslesaudiooutput.cpp
