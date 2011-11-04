CONFIG += testcase
TARGET = tst_qdeclarativevideo

QT += multimedia-private declarative testlib
CONFIG += no_private_qt_headers_warning

HEADERS += \
        $$QT.multimedia.sources/../imports/multimedia/qdeclarativevideo_p.h \
        $$QT.multimedia.sources/../imports/multimedia/qdeclarativemediabase_p.h \
        $$QT.multimedia.sources/../imports/multimedia/qdeclarativemediametadata_p.h

SOURCES += \
        tst_qdeclarativevideo.cpp \
        $$QT.multimedia.sources/../imports/multimedia/qdeclarativevideo.cpp \
        $$QT.multimedia.sources/../imports/multimedia/qdeclarativemediabase.cpp

INCLUDEPATH += $$QT.multimedia.sources/../imports/multimedia
QT+=widgets
