TEMPLATE = app
TARGET = camera

INCLUDEPATH+=../../src/multimedia \
             ../../src/multimedia/video

include(../mobility_examples.pri)

CONFIG += mobility
MOBILITY = multimedia

HEADERS = \
    camera.h \
    imagesettings.h \
    videosettings.h

SOURCES = \
    main.cpp \
    camera.cpp \
    imagesettings.cpp \
    videosettings.cpp

FORMS += \
    camera.ui \
    videosettings.ui \
    imagesettings.ui

symbian {
    include(camerakeyevent_symbian/camerakeyevent_symbian.pri)
    TARGET.CAPABILITY += UserEnvironment WriteUserData ReadUserData
    TARGET.EPOCHEAPSIZE = 0x20000 0x3000000
    LIBS += -lavkon -leiksrv -lcone -leikcore
}
