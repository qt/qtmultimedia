INCLUDEPATH += $$PWD

LIBS += -lgdi32 -luser32
QMAKE_USE += wmf

HEADERS += \
    $$PWD/mfplayerservice_p.h \
    $$PWD/mfplayersession_p.h \
    $$PWD/mfplayercontrol_p.h \
    $$PWD/mfvideorenderercontrol_p.h \
    $$PWD/mfmetadata_p.h \
    $$PWD/mfevrvideowindowcontrol_p.h \
    $$PWD/samplegrabber_p.h \
    $$PWD/mftvideo_p.h \
    $$PWD/mfactivate_p.h

SOURCES += \
    $$PWD/mfplayerservice.cpp \
    $$PWD/mfplayersession.cpp \
    $$PWD/mfplayercontrol.cpp \
    $$PWD/mfvideorenderercontrol.cpp \
    $$PWD/mfmetadata.cpp \
    $$PWD/mfevrvideowindowcontrol.cpp \
    $$PWD/samplegrabber.cpp \
    $$PWD/mftvideo.cpp \
    $$PWD/mfactivate.cpp
