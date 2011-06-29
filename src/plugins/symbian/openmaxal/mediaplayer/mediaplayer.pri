INCLUDEPATH += $$PWD

LIBS += \
    -lws32 \
    -lcone

#DEFINES += USE_VIDEOPLAYERUTILITY

HEADERS += \
    $$PWD/qxametadatacontrol.h \
    $$PWD/qxamediastreamscontrol.h \
    $$PWD/qxamediaplayercontrol.h \
    $$PWD/qxaplaymediaservice.h \
    $$PWD/qxaplaysession.h \
    $$PWD/xaplaysessioncommon.h \
    $$PWD/qxavideowidgetcontrol.h \
    $$PWD/qxavideowindowcontrol.h \
    $$PWD/qxawidget.h \
    $$PWD/xaplaysessionimpl.h
    
SOURCES += \
    $$PWD/qxamediaplayercontrol.cpp \
    $$PWD/qxametadatacontrol.cpp \
    $$PWD/qxamediastreamscontrol.cpp \
    $$PWD/qxaplaymediaservice.cpp \
    $$PWD/qxaplaysession.cpp \
    $$PWD/qxavideowidgetcontrol.cpp \
    $$PWD/qxavideowindowcontrol.cpp \
    $$PWD/qxawidget.cpp \
    $$PWD/xaplaysessionimpl.cpp

# check for USE_VIDEOPLAYERUTILITY
contains(DEFINES, USE_VIDEOPLAYERUTILITY) {
    message("Using VideoPlayerUtility instead of OpenMAX AL.")
    LIBS += -lmediaclientvideo
}
