CONFIG += testcase
TARGET = tst_qcameraviewfinder

QT += multimedia-private multimediawidgets-private testlib
CONFIG += no_private_qt_headers_warning

include (../qmultimedia_common/mock.pri)
include (../qmultimedia_common/mockcamera.pri)

SOURCES += tst_qcameraviewfinder.cpp
QT+=widgets
