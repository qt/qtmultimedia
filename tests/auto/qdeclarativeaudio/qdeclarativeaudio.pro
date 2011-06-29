load(qttest_p4)

QT += multimediakit-private declarative

# TARGET = tst_qdeclarativeaudio
# CONFIG += testcase

HEADERS += \
        $$QT.multimediakit.sources/../imports/multimedia/qdeclarativeaudio_p.h \
        $$QT.multimediakit.sources/../imports/multimedia/qdeclarativemediabase_p.h \
        $$QT.multimediakit.sources/../imports/multimedia/qdeclarativemediametadata_p.h

SOURCES += \
        tst_qdeclarativeaudio.cpp \
        $$QT.multimediakit.sources/../imports/multimedia/qdeclarativeaudio.cpp \
        $$QT.multimediakit.sources/../imports/multimedia/qdeclarativemediabase.cpp

INCLUDEPATH += $$QT.multimediakit.sources/../imports/multimedia
