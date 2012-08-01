TARGET = tst_qaudiooutput

QT += core multimedia-private testlib
CONFIG += no_private_qt_headers_warning

# This is more of a system test
CONFIG += testcase

HEADERS += wavheader.h
SOURCES += wavheader.cpp tst_qaudiooutput.cpp

mac: CONFIG += insignificant_test
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
