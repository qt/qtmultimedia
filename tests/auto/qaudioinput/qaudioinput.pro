load(qttest_p4)

QT += core multimediakit-private
CONFIG += no_private_qt_headers_warning

# This is more of a system test
CONFIG -= testcase

DEFINES += SRCDIR=\\\"$$PWD/\\\"

HEADERS += wavheader.h
SOURCES += wavheader.cpp tst_qaudioinput.cpp

maemo*:CONFIG += insignificant_test
