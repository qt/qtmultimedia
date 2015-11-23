INCLUDEPATH += $$PWD

LIBS += -lstrmiids -ldmoguids -luuid -lmsdmo -lgdi32 -luser32 -lole32 -loleaut32 -lMf -lMfuuid -lMfplat -lPropsys

DEFINES += QMEDIA_MEDIAFOUNDATION_PLAYER

HEADERS += \
    $$PWD/mfplayerservice.h \
    $$PWD/mfplayersession.h \
    $$PWD/mfplayercontrol.h \
    $$PWD/mfvideorenderercontrol.h \
    $$PWD/mfaudioendpointcontrol.h \
    $$PWD/mfmetadatacontrol.h \
    $$PWD/mfaudioprobecontrol.h \
    $$PWD/mfvideoprobecontrol.h \
    $$PWD/mfevrvideowindowcontrol.h

SOURCES += \
    $$PWD/mfplayerservice.cpp \
    $$PWD/mfplayersession.cpp \
    $$PWD/mfplayercontrol.cpp \
    $$PWD/mfvideorenderercontrol.cpp \
    $$PWD/mfaudioendpointcontrol.cpp \
    $$PWD/mfmetadatacontrol.cpp \
    $$PWD/mfaudioprobecontrol.cpp \
    $$PWD/mfvideoprobecontrol.cpp \
    $$PWD/mfevrvideowindowcontrol.cpp

include($$PWD/../../common/evr.pri)
