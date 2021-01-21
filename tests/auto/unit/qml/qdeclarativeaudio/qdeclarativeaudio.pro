CONFIG += testcase
TARGET = tst_qdeclarativeaudio

include (../../mockbackend/mockbackend.pri)
QT += multimedia-private qml testlib qtmultimediaquicktools-private

HEADERS += \
        ../../../../../src/imports/multimedia/qdeclarativeaudio_p.h \
        ../../../../../src/imports/multimedia/qdeclarativeplaylist_p.h \
        ../../../../../src/imports/multimedia/qdeclarativemediametadata_p.h

SOURCES += \
        tst_qdeclarativeaudio.cpp \
        ../../../../../src/imports/multimedia/qdeclarativeplaylist.cpp \
        ../../../../../src/imports/multimedia/qdeclarativeaudio.cpp

INCLUDEPATH += ../../../../../src/imports/multimedia


