TARGET = tst_qaudioinput

QT += core multimedia-private testlib
CONFIG += no_private_qt_headers_warning

# This is more of a system test
# CONFIG += testcase

HEADERS += wavheader.h
SOURCES += wavheader.cpp tst_qaudioinput.cpp
