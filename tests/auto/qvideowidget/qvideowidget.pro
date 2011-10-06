load(qttest_p4)

QT += multimedia-private multimediawidgets-private
CONFIG += no_private_qt_headers_warning

SOURCES += tst_qvideowidget.cpp

# QPA seems to break some assumptions
qpa:CONFIG += insignificant_test
QT+=widgets
