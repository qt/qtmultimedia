INCLUDEPATH += $$PWD

LIBS += -lstrmiids -ldmoguids -luuid -lole32 -loleaut32
!wince: LIBS += -lmsdmo -lgdi32


qtHaveModule(widgets): QT += widgets

DEFINES += QMEDIA_DIRECTSHOW_PLAYER

HEADERS += \
        $$PWD/directshoweventloop.h \
        $$PWD/directshowglobal.h \
        $$PWD/directshowioreader.h \
        $$PWD/directshowiosource.h \
        $$PWD/directshowmediatype.h \
        $$PWD/directshowmediatypelist.h \
        $$PWD/directshowpinenum.h \
        $$PWD/directshowplayercontrol.h \
        $$PWD/directshowplayerservice.h \
        $$PWD/directshowsamplescheduler.h \
        $$PWD/directshowvideorenderercontrol.h \
        $$PWD/mediasamplevideobuffer.h \
        $$PWD/videosurfacefilter.h

SOURCES += \
        $$PWD/directshoweventloop.cpp \
        $$PWD/directshowioreader.cpp \
        $$PWD/directshowiosource.cpp \
        $$PWD/directshowmediatype.cpp \
        $$PWD/directshowmediatypelist.cpp \
        $$PWD/directshowpinenum.cpp \
        $$PWD/directshowplayercontrol.cpp \
        $$PWD/directshowplayerservice.cpp \
        $$PWD/directshowsamplescheduler.cpp \
        $$PWD/directshowvideorenderercontrol.cpp \
        $$PWD/mediasamplevideobuffer.cpp \
        $$PWD/videosurfacefilter.cpp

!wince {
HEADERS += \
        $$PWD/directshowaudioendpointcontrol.h \
        $$PWD/directshowmetadatacontrol.h \
        $$PWD/vmr9videowindowcontrol.h

SOURCES += \
        $$PWD/directshowaudioendpointcontrol.cpp \
        $$PWD/directshowmetadatacontrol.cpp \
        $$PWD/vmr9videowindowcontrol.cpp
}

config_evr {
    DEFINES += HAVE_EVR

    include($$PWD/../../common/evr.pri)

    HEADERS += \
        $$PWD/directshowevrvideowindowcontrol.h

    SOURCES += \
        $$PWD/directshowevrvideowindowcontrol.cpp
}

config_wshellitem {
    QT += core-private
} else {
    DEFINES += QT_NO_SHELLITEM
}
