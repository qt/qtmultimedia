INCLUDEPATH += $$PWD

LIBS += -lstrmiids -ldmoguids -luuid -lmsdmo -lole32 -loleaut32 -lMf -lMfuuid -lMfplat -lPropsys

DEFINES += QMEDIA_MEDIAFOUNDATION_PLAYER

HEADERS += \
    $$PWD/mfplayerservice.h \
    $$PWD/mfplayersession.h \
    $$PWD/mfstream.h \
    $$PWD/sourceresolver.h \
    $$PWD/mfplayercontrol.h \
    $$PWD/mfvideorenderercontrol.h \
    $$PWD/mfaudioendpointcontrol.h \
    $$PWD/mfmetadatacontrol.h

SOURCES += \
    $$PWD/mfplayerservice.cpp \
    $$PWD/mfplayersession.cpp \
    $$PWD/mfstream.cpp \
    $$PWD/sourceresolver.cpp \
    $$PWD/mfplayercontrol.cpp \
    $$PWD/mfvideorenderercontrol.cpp \
    $$PWD/mfaudioendpointcontrol.cpp \
    $$PWD/mfmetadatacontrol.cpp

contains(config_test_widgets, yes):!simulator {
    HEADERS += $$PWD/evr9videowindowcontrol.h
    SOURCES += $$PWD/evr9videowindowcontrol.cpp
}
