CONFIG += testcase
TARGET = tst_qmediarecorder

QT += multimedia-private testlib
CONFIG += no_private_qt_headers_warning

include (../qmultimedia_common/mock.pri)
include (../qmultimedia_common/mockrecorder.pri)

HEADERS += tst_qmediarecorder.h
SOURCES += main.cpp tst_qmediarecorder.cpp

