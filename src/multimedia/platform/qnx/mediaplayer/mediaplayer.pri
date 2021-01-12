INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/mmrenderermediaplayercontrol_p.h \
    $$PWD/mmrenderermediaplayerservice_p.h \
    $$PWD/mmrenderermetadata_p.h \
    $$PWD/mmrenderermetadatareadercontrol_p.h \
    $$PWD/mmrendererplayervideorenderercontrol_p.h \
    $$PWD/mmrendererutil_p.h \
    $$PWD/mmrenderervideowindowcontrol_p.h \
    $$PWD/mmreventmediaplayercontrol_p.h \
    $$PWD/mmrevent_p.hread.h
SOURCES += \
    $$PWD/mmrenderermediaplayercontrol.cpp \
    $$PWD/mmrenderermediaplayerservice.cpp \
    $$PWD/mmrenderermetadata.cpp \
    $$PWD/mmrenderermetadatareadercontrol.cpp \
    $$PWD/mmrendererplayervideorenderercontrol.cpp \
    $$PWD/mmrendererutil.cpp \
    $$PWD/mmrenderervideowindowcontrol.cpp \
    $$PWD/mmreventmediaplayercontrol.cpp \
    $$PWD/mmrevent_p.hread.cpp

QMAKE_USE += mmrenderer
