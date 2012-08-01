CONFIG += testcase
TARGET = tst_qmediaresource

QT += network multimedia-private testlib
CONFIG += no_private_qt_headers_warning

SOURCES += tst_qmediaresource.cpp

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
