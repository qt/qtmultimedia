load(qttest_p4)

QT += multimedia-private multimediawidgets-private
CONFIG += no_private_qt_headers_warning

SOURCES += tst_qgraphicsvideoitem.cpp

# QPA minimal crashes with this test in QBackingStore
qpa:CONFIG += insignificant_test
QT+=widgets
