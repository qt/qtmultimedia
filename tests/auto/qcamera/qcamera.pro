CONFIG += testcase
TARGET = tst_qcamera

QT += multimedia-private testlib
CONFIG += no_private_qt_headers_warning

include (../qmultimedia_common/mock.pri)
include (../qmultimedia_common/mockcamera.pri)

SOURCES += tst_qcamera.cpp

maemo*:CONFIG += insignificant_test
