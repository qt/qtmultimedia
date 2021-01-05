CONFIG += testcase
TARGET = tst_qmediasource
QT += multimedia-private testlib

include (../qmultimedia_common/mockrecorder.pri)
include (../qmultimedia_common/mock.pri)

SOURCES += tst_qmediasource.cpp
