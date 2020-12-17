LIBS += -lasound

HEADERS += audio/qnx/qnxaudiointerface_p.h \
           audio/qnx/qnxaudiodeviceinfo_p.h \
           audio/qnx/qnxaudioinput_p.h \
           audio/qnx/qnxaudiooutput_p.h \
           audio/qnx/qnxaudioutils_p.h

SOURCES += audio/qnx/qnxaudiointerface.cpp \
           audio/qnx/qnxaudiodeviceinfo.cpp \
           audio/qnx/qnxaudioinput.cpp \
           audio/qnx/qnxaudiooutput.cpp \
           audio/qnx/qnxaudioutils.cpp
