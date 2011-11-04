TARGET = tst_qmediaplayerbackend

QT += multimedia-private testlib
CONFIG += no_private_qt_headers_warning

# This is more of a system test
# CONFIG += testcase

DEFINES += TESTDATA_DIR=\\\"$$PWD/\\\"

SOURCES += \
    tst_qmediaplayerbackend.cpp
