load(qttest_p4)

QT += core multimediakit-private

# TARGET = tst_qaudioinput

# This is more of a system test
CONFIG -= testcase

DEFINES += SRCDIR=\\\"$$PWD/\\\"

HEADERS += wavheader.h
SOURCES += wavheader.cpp tst_qaudioinput.cpp

maemo*:CONFIG += insignificant_test
