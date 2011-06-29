INCLUDEPATH += $$PWD

# Input
HEADERS += \
    $$PWD/qxaradiomediaservice.h \
    $$PWD/qxaradiosession.h \
    $$PWD/qxaradiocontrol.h \
    $$PWD/xaradiosessionimpl.h \
    $$PWD/xaradiosessionimplobserver.h

SOURCES += \
    $$PWD/qxaradiomediaservice.cpp \
    $$PWD/qxaradiosession.cpp \
    $$PWD/qxaradiocontrol.cpp \
    $$PWD/xaradiosessionimpl.cpp
 
LIBS += \
    -lbafl
