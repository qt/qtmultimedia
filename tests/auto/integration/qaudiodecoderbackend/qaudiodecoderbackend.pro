TARGET = tst_qaudiodecoderbackend

QT += multimedia multimedia-private testlib
CONFIG += no_private_qt_headers_warning

# This is more of a system test
# CONFIG += testcase

INCLUDEPATH += \
    ../../../../src/multimedia/audio

DEFINES += TESTDATA_DIR=\\\"$$PWD/\\\"

SOURCES += \
    tst_qaudiodecoderbackend.cpp
