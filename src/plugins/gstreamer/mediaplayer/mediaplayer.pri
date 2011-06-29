INCLUDEPATH += $$PWD

DEFINES += QMEDIA_GSTREAMER_PLAYER

contains(gstreamer-appsrc_enabled, yes) {
    HEADERS += $$PWD/qgstappsrc.h
    SOURCES += $$PWD/qgstappsrc.cpp

    DEFINES += HAVE_GST_APPSRC

    LIBS += -lgstapp-0.10
}

HEADERS += \
    $$PWD/qgstreamerplayercontrol.h \
    $$PWD/qgstreamerplayerservice.h \
    $$PWD/qgstreamerplayersession.h \
    $$PWD/qgstreamerstreamscontrol.h \
    $$PWD/qgstreamermetadataprovider.h \
    $$PWD/playerresourcepolicy.h

SOURCES += \
    $$PWD/qgstreamerplayercontrol.cpp \
    $$PWD/qgstreamerplayerservice.cpp \
    $$PWD/qgstreamerplayersession.cpp \
    $$PWD/qgstreamerstreamscontrol.cpp \
    $$PWD/qgstreamermetadataprovider.cpp \
    $$PWD/playerresourcepolicy.cpp


