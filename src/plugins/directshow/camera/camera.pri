INCLUDEPATH += $$PWD

DEFINES += QMEDIA_DIRECTSHOW_CAMERA

win32-g++: DEFINES += QT_NO_WMSDK

win32: DEFINES += _CRT_SECURE_NO_WARNINGS

HEADERS += \
    $$PWD/dscameraservice.h \
    $$PWD/dscameracontrol.h \
    $$PWD/dsvideorenderer.h \
    $$PWD/dsvideodevicecontrol.h \
    $$PWD/dsimagecapturecontrol.h \
    $$PWD/dscamerasession.h \
    $$PWD/dscameraservice.h \
    $$PWD/directshowglobal.h

SOURCES += \
    $$PWD/dscameraservice.cpp \
    $$PWD/dscameracontrol.cpp \
    $$PWD/dsvideorenderer.cpp \
    $$PWD/dsvideodevicecontrol.cpp \
    $$PWD/dsimagecapturecontrol.cpp \
    $$PWD/dscamerasession.cpp

contains(config_test_widgets, yes) {
    HEADERS += $$PWD/dsvideowidgetcontrol.h
    SOURCES += $$PWD/dsvideowidgetcontrol.cpp
}

INCLUDEPATH += $(DXSDK_DIR)/include
LIBS += -lstrmiids -ldmoguids -luuid -lmsdmo -lole32 -loleaut32
