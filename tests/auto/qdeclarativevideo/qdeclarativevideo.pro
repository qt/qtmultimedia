load(qttest_p4)

QT += multimediakit-private multimediakitwidgets-private declarative
CONFIG += no_private_qt_headers_warning

HEADERS += \
        $$QT.multimediakit.sources/../imports/multimedia/qdeclarativevideo_p.h \
        $$QT.multimediakit.sources/../imports/multimedia/qdeclarativemediabase_p.h \
        $$QT.multimediakit.sources/../imports/multimedia/qdeclarativemediametadata_p.h

SOURCES += \
        tst_qdeclarativevideo.cpp \
        $$QT.multimediakit.sources/../imports/multimedia/qdeclarativevideo.cpp \
        $$QT.multimediakit.sources/../imports/multimedia/qdeclarativemediabase.cpp

INCLUDEPATH += $$QT.multimediakit.sources/../imports/multimedia
