load(qttest_p4)

QT += multimediakit-private multimediakitwidgets-private
CONFIG += no_private_qt_headers_warning

# This is more of a system test
CONFIG -= testcase

SOURCES += tst_qcamerabackend.cpp

maemo*:CONFIG += insignificant_test
