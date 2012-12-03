CONFIG += testcase
TARGET = tst_qmediaplaylist

include (../qmultimedia_common/mockplaylist.pri)

QT += multimedia-private testlib
CONFIG += no_private_qt_headers_warning

HEADERS += \
    ../../../../src/plugins/m3u/qm3uhandler.h

SOURCES += \
    tst_qmediaplaylist.cpp \
    ../../../../src/plugins/m3u/qm3uhandler.cpp

INCLUDEPATH += ../../../../src/plugins/m3u

TESTDATA += testdata/*
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
