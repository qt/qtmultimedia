TARGET = tst_qdeclarativevideooutput

QT += multimedia-private qml testlib quick
CONFIG += testcase

OTHER_FILES += \
        ../../../../src/imports/multimedia/qdeclarativevideooutput_p.h

SOURCES += \
        tst_qdeclarativevideooutput.cpp

INCLUDEPATH += ../../../../src/imports/multimedia
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
