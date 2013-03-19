INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/bbmediaplayercontrol.h \
    $$PWD/bbmediaplayerservice.h \
    $$PWD/bbmetadata.h \
    $$PWD/bbmetadatareadercontrol.h \
    $$PWD/bbplayervideorenderercontrol.h \
    $$PWD/bbutil.h \
    $$PWD/bbvideowindowcontrol.h

SOURCES += \
    $$PWD/bbmediaplayercontrol.cpp \
    $$PWD/bbmediaplayerservice.cpp \
    $$PWD/bbmetadata.cpp \
    $$PWD/bbmetadatareadercontrol.cpp \
    $$PWD/bbplayervideorenderercontrol.cpp \
    $$PWD/bbutil.cpp \
    $$PWD/bbvideowindowcontrol.cpp

LIBS += -lmmrndclient -lstrm
