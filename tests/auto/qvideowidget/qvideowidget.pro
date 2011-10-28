CONFIG += testcase
TARGET = tst_qvideowidget

QT += multimedia-private multimediawidgets-private testlib
CONFIG += no_private_qt_headers_warning

SOURCES += tst_qvideowidget.cpp

# QPA seems to break some assumptions
CONFIG += insignificant_test
QT+=widgets
