CONFIG += testcase
TARGET = tst_qvideoprobe

QT += multimedia-private testlib

SOURCES += tst_qvideoprobe.cpp

include (../../mockbackend/mockbackend.pri)
