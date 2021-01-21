TARGET = tst_qaudiodecoder
CONFIG += testcase

QT += multimedia multimedia-private testlib gui

include (../../mockbackend/mockbackend.pri)

SOURCES += tst_qaudiodecoder.cpp
