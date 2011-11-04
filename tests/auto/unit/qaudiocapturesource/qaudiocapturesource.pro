CONFIG += testcase
TARGET = tst_qaudiocapturesource

QT += multimedia-private testlib
CONFIG += no_private_qt_headers_warning

SOURCES += tst_qaudiocapturesource.cpp

include (../qmultimedia_common/mockrecorder.pri)
include (../qmultimedia_common/mock.pri)

