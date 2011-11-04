CONFIG += testcase
TARGET = tst_qcamerawidgets

QT += multimedia-private multimediawidgets-private testlib
CONFIG += no_private_qt_headers_warning

include (../qmultimedia_common/mock.pri)
include (../qmultimedia_common/mockcamera.pri)

SOURCES += tst_qcamerawidgets.cpp

maemo*:CONFIG += insignificant_test
QT+=widgets
