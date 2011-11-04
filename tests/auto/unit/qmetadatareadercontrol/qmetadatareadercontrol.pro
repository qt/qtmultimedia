CONFIG += testcase
TARGET = tst_qmetadatareadercontrol

QT += multimedia-private testlib
CONFIG += no_private_qt_headers_warning

SOURCES += tst_qmetadatareadercontrol.cpp

include (../qmultimedia_common/mockcontainer.pri)

