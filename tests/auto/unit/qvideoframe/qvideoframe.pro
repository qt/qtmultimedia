CONFIG += testcase
TARGET = tst_qvideoframe

QT += core multimedia-private testlib
CONFIG += no_private_qt_headers_warning

SOURCES += tst_qvideoframe.cpp

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
