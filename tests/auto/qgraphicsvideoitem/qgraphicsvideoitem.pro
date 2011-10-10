CONFIG += testcase
TARGET = tst_qgraphicsvideoitem

QT += multimedia-private multimediawidgets-private testlib
CONFIG += no_private_qt_headers_warning

SOURCES += tst_qgraphicsvideoitem.cpp

# QPA minimal crashes with this test in QBackingStore
qpa:CONFIG += insignificant_test
QT+=widgets
