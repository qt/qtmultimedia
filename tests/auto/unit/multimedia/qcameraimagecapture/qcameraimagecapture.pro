CONFIG += testcase
TARGET = tst_qcameraimagecapture

QT += multimedia-private testlib

SOURCES += \
    tst_qcameraimagecapture.cpp

include (../../mockbackend/mockbackend.pri)
