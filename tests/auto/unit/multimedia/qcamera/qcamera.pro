CONFIG += testcase
TARGET = tst_qcamera

QT += multimedia-private testlib

include (../../mockbackend/mockbackend.pri)

SOURCES += tst_qcamera.cpp
