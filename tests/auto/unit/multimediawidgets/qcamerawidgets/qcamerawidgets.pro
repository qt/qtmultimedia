CONFIG += testcase
TARGET = tst_qcamerawidgets

QT += multimedia-private multimediawidgets-private testlib

include (../../mockbackend/mockbackend.pri)

SOURCES += tst_qcamerawidgets.cpp

QT+=widgets
