CONFIG += testcase
TARGET = tst_qmediaplayerwidgets

QT += network multimedia-private multimediawidgets-private testlib widgets
CONFIG += no_private_qt_headers_warning

HEADERS += tst_qmediaplayerwidgets.h
SOURCES += main.cpp tst_qmediaplayerwidgets.cpp

include (../qmultimedia_common/mock.pri)
include (../qmultimedia_common/mockplayer.pri)
