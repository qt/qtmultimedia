load(qttest_p4)

QT += multimediakit-private multimediakitwidgets-private
CONFIG += no_private_qt_headers_warning

SOURCES += tst_qgraphicsvideoitem.cpp

# QPA minimal crashes with this test in QBackingStore
qpa:CONFIG += insignificant_test
QT+=widgets
