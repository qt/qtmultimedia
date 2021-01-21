CONFIG += testcase
TARGET = tst_qaudioprobe

QT += multimedia-private testlib

SOURCES += tst_qaudioprobe.cpp

include (../../mockbackend/mockbackend.pri)

