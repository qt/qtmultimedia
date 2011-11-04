CONFIG += testcase
TARGET = tst_qradiodata

QT += multimedia-private testlib
CONFIG += no_private_qt_headers_warning

HEADERS += tst_qradiodata.h
SOURCES += main.cpp tst_qradiodata.cpp

include (../qmultimedia_common/mock.pri)
include (../qmultimedia_common/mockradio.pri)
