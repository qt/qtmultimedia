LIBS += -lOpenSLES

HEADERS += \
    $$PWD/qopenslesengine_p.h \
    $$PWD/qopenslesdeviceinfo_p.h \
    $$PWD/qopenslesaudioinput_p.h \
    $$PWD/qopenslesaudiooutput_p.h

SOURCES += \
    $$PWD/qopenslesengine.cpp \
    $$PWD/qopenslesdeviceinfo.cpp \
    $$PWD/qopenslesaudioinput.cpp \
    $$PWD/qopenslesaudiooutput.cpp
