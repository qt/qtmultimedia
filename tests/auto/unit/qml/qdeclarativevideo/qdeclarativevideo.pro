CONFIG += testcase
TARGET = tst_qdeclarativevideo

QT += multimedia-private qml testlib

include (../../mockbackend/mockbackend.pri)

HEADERS += \
        ../../../../src/imports/multimedia/qdeclarativevideo_p.h \
        ../../../../src/imports/multimedia/qdeclarativemediabase_p.h \
        ../../../../src/imports/multimedia/qdeclarativemediametadata_p.h

SOURCES += \
        tst_qdeclarativevideo.cpp \
        ../../../../src/imports/multimedia/qdeclarativevideo.cpp \
        ../../../../src/imports/multimedia/qdeclarativemediabase.cpp

INCLUDEPATH += ../../../../src/imports/multimedia
