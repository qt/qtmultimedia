CONFIG += testcase
TARGET = tst_qdeclarativevideooutput

QT += multimedia-private qml testlib quick
CONFIG += no_private_qt_headers_warning

OTHER_FILES += \
        ../../../../src/imports/multimedia/qdeclarativevideooutput_p.h

SOURCES += \
        tst_qdeclarativevideooutput.cpp

INCLUDEPATH += ../../../../src/imports/multimedia
