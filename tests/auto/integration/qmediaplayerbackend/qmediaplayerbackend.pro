TARGET = tst_qmediaplayerbackend

QT += multimedia-private testlib
CONFIG += no_private_qt_headers_warning

# This is more of a system test
# CONFIG += testcase


SOURCES += \
    tst_qmediaplayerbackend.cpp

TESTDATA += testdata/*
