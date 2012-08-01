CONFIG += testcase
TARGET = tst_qabstractvideobuffer

QT += core multimedia-private testlib
CONFIG += no_private_qt_headers_warning

SOURCES += tst_qabstractvideobuffer.cpp

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
