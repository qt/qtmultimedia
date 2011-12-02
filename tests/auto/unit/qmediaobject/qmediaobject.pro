CONFIG += testcase no_private_qt_headers_warning
TARGET = tst_qmediaobject
QT += multimedia-private testlib

include (../qmultimedia_common/mockrecorder.pri)

SOURCES += tst_qmediaobject.cpp
