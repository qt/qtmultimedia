CONFIG += testcase
TARGET = tst_qmediasink

QT += multimedia-private testlib

SOURCES += \
    tst_qmediasink.cpp

include (../../mockbackend/mockbackend.pri)
