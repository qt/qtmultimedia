INCLUDEPATH += $$PWD

# Input
HEADERS += \
    $$PWD/qxarecordmediaservice.h \
    $$PWD/qxarecordsession.h \
    $$PWD/qxaaudioendpointselector.h \
    $$PWD/qxaaudioencodercontrol.h \
    $$PWD/qxamediacontainercontrol.h \
    $$PWD/qxamediarecordercontrol.h \
    $$PWD/xarecordsessionimpl.h \
    $$PWD/xarecordsessioncommon.h

SOURCES += \
    $$PWD/qxarecordmediaservice.cpp \
    $$PWD/qxarecordsession.cpp \
    $$PWD/qxaaudioendpointselector.cpp \
    $$PWD/qxaaudioencodercontrol.cpp \
    $$PWD/qxamediacontainercontrol.cpp \
    $$PWD/qxamediarecordercontrol.cpp \
    $$PWD/xarecordsessionimpl.cpp

LIBS += \
    -lbafl
