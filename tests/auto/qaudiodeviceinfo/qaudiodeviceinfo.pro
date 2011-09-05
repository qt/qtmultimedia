load(qttest_p4)

QT += core multimediakit-private
CONFIG += no_private_qt_headers_warning

# This is more of a system test
CONFIG -= testcase

SOURCES += tst_qaudiodeviceinfo.cpp

