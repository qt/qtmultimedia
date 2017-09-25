INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/mmrenderermediaplayercontrol.h \
    $$PWD/mmrenderermediaplayerservice.h \
    $$PWD/mmrenderermetadata.h \
    $$PWD/mmrenderermetadatareadercontrol.h \
    $$PWD/mmrendererplayervideorenderercontrol.h \
    $$PWD/mmrendererutil.h \
    $$PWD/mmrenderervideowindowcontrol.h \
    $$PWD/mmreventmediaplayercontrol.h \
    $$PWD/mmreventthread.h
SOURCES += \
    $$PWD/mmrenderermediaplayercontrol.cpp \
    $$PWD/mmrenderermediaplayerservice.cpp \
    $$PWD/mmrenderermetadata.cpp \
    $$PWD/mmrenderermetadatareadercontrol.cpp \
    $$PWD/mmrendererplayervideorenderercontrol.cpp \
    $$PWD/mmrendererutil.cpp \
    $$PWD/mmrenderervideowindowcontrol.cpp \
    $$PWD/mmreventmediaplayercontrol.cpp \
    $$PWD/mmreventthread.cpp

QMAKE_USE += mmrenderer pps
