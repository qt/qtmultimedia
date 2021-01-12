LIBS += -lasound

HEADERS += platform/qnx/qnxaudiointerface_p.h \
           platform/qnx/qnxaudiodeviceinfo_p.h \
           platform/qnx/qnxaudioinput_p.h \
           platform/qnx/qnxaudiooutput_p.h \
           platform/qnx/qnxaudioutils_p.h

SOURCES += platform/qnx/qnxaudiointerface.cpp \
           platform/qnx/qnxaudiodeviceinfo.cpp \
           platform/qnx/qnxaudioinput.cpp \
           platform/qnx/qnxaudiooutput.cpp \
           platform/qnx/qnxaudioutils.cpp
