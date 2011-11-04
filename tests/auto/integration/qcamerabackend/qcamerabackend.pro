CONFIG += testcase
TARGET = tst_qcamerabackend

QT += multimedia-private multimediawidgets-private testlib
CONFIG += no_private_qt_headers_warning

# This is more of a system test
# CONFIG += testcase

SOURCES += tst_qcamerabackend.cpp
