CONFIG += testcase
TARGET = tst_qmediasink

QT += multimedia-private testlib

SOURCES += \
    tst_qmediasink.cpp

include (../qmultimedia_common/mock.pri)
include (../qmultimedia_common/mockrecorder.pri)
