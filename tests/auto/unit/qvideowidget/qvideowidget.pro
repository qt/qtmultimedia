CONFIG += testcase
TARGET = tst_qvideowidget

QT += multimedia-private multimediawidgets-private testlib
CONFIG += no_private_qt_headers_warning

SOURCES += tst_qvideowidget.cpp

QT+=widgets
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
