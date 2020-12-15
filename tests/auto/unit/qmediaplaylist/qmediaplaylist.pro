CONFIG += testcase
TARGET = tst_qmediaplaylist

QT += multimedia-private testlib

SOURCES += \
    tst_qmediaplaylist.cpp

INCLUDEPATH += ../../../../src/plugins/m3u

TESTDATA += testdata/*
