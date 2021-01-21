CONFIG += testcase
TARGET = tst_qmediaplayer
QT += network multimedia-private testlib
SOURCES += tst_qmediaplayer.cpp
RESOURCES += testdata.qrc

include (../../mockbackend/mockbackend.pri)
