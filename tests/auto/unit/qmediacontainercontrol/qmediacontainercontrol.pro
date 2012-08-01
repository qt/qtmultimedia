CONFIG += testcase
TARGET = tst_qmediacontainercontrol

QT += multimedia-private testlib
CONFIG += no_private_qt_headers_warning

SOURCES += tst_qmediacontainercontrol.cpp

include (../qmultimedia_common/mockcontainer.pri)

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
