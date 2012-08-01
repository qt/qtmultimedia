CONFIG += testcase
TARGET = tst_qaudionamespace

QT += core multimedia-private testlib
CONFIG += no_private_qt_headers_warning

SOURCES += tst_qaudionamespace.cpp

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
