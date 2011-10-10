CONFIG += testcase
TARGET = tst_qmediastreamscontrol

QT += multimedia-private testlib
CONFIG += no_private_qt_headers_warning

SOURCES += \
        tst_qmediastreamscontrol.cpp

include(../multimedia_common.pri)
