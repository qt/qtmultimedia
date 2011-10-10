CONFIG += testcase
TARGET = tst_qmediaobject

QT += multimedia-private testlib
CONFIG += no_private_qt_headers_warning

include (../qmultimedia_common/mockrecorder.pri)

HEADERS+= tst_qmediaobject.h
SOURCES += main.cpp tst_qmediaobject.cpp

