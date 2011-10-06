load(qttest_p4)

QT += multimedia-private multimediawidgets-private
CONFIG += no_private_qt_headers_warning

include (../qmultimedia_common/mock.pri)
include (../qmultimedia_common/mockcamera.pri)

SOURCES += tst_qcamerawidgets.cpp

maemo*:CONFIG += insignificant_test
QT+=widgets
