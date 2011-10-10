CONFIG += testcase
TARGET = tst_qmetadatawritercontrol

QT += multimedia-private testlib
CONFIG += no_private_qt_headers_warning

SOURCES += tst_qmetadatawritercontrol.cpp

include (../qmultimedia_common/mockcontainer.pri)
