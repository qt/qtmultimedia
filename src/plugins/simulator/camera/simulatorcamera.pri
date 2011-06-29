INCLUDEPATH += $$PWD \
    $${SOURCE_DIR}/src/multimedia

INCLUDEPATH += camera

HEADERS += \
    $$PWD/simulatorcameraservice.h \
    $$PWD/simulatorcamerasession.h \
    $$PWD/simulatorcameracontrol.h \
    $$PWD/simulatorvideorenderercontrol.h \
    $$PWD/simulatorvideoinputdevicecontrol.h \
    $$PWD/simulatorcameraimagecapturecontrol.h \
    $$PWD/simulatorcameraexposurecontrol.h \
    $$PWD/simulatorcamerasettings.h

SOURCES += \
    $$PWD/simulatorcameraservice.cpp \
    $$PWD/simulatorcamerasession.cpp \
    $$PWD/simulatorcameracontrol.cpp \
    $$PWD/simulatorvideorenderercontrol.cpp \
    $$PWD/simulatorvideoinputdevicecontrol.cpp \
    $$PWD/simulatorcameraimagecapturecontrol.cpp \
    $$PWD/simulatorcameraexposurecontrol.cpp \
    $$PWD/simulatorcamerasettings.cpp

