CONFIG += testcase
TARGET = tst_qaudiorecorder

QT += multimedia-private testlib

SOURCES += tst_qaudiorecorder.cpp

include (../../mockbackend/mockbackend.pri)

