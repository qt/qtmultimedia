TARGET = tst_qcamerabackend

QT += multimedia-private testlib

# This is more of a system test
CONFIG += testcase

SOURCES += tst_qcamerabackend.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
