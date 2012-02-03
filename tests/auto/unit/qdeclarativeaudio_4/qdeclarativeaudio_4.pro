CONFIG += testcase
TARGET = tst_qdeclarativeaudio_4

QT += multimedia-private declarative testlib
CONFIG += no_private_qt_headers_warning

HEADERS += \
        $$QT.multimedia.sources/../imports/multimedia/qdeclarativeaudio_p_4.h \
        $$QT.multimedia.sources/../imports/multimedia/qdeclarativemediabase_p_4.h \
        $$QT.multimedia.sources/../imports/multimedia/qdeclarativemediametadata_p.h

SOURCES += \
        tst_qdeclarativeaudio_4.cpp \
        $$QT.multimedia.sources/../imports/multimedia/qdeclarativeaudio_4.cpp \
        $$QT.multimedia.sources/../imports/multimedia/qdeclarativemediabase_4.cpp

INCLUDEPATH += $$QT.multimedia.sources/../imports/multimedia
