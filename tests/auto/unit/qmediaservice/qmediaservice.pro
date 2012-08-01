CONFIG += testcase
TARGET = tst_qmediaservice

QT += multimedia-private testlib
CONFIG += no_private_qt_headers_warning

SOURCES += tst_qmediaservice.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
