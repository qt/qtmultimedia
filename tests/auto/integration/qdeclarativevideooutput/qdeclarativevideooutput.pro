TARGET = tst_qdeclarativevideooutput

QT += multimedia-private qml testlib quick qtmultimediaquicktools-private
CONFIG += testcase

RESOURCES += qml.qrc

SOURCES += \
        tst_qdeclarativevideooutput.cpp

INCLUDEPATH += ../../../../src/imports/multimedia
