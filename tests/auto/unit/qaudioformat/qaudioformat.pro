CONFIG += testcase
TARGET = tst_qaudioformat

QT += core multimedia-private testlib
CONFIG += no_private_qt_headers_warning

SOURCES += tst_qaudioformat.cpp

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
