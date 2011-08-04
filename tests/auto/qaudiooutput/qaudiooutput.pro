load(qttest_p4)

QT += core multimediakit-private

# TARGET = tst_qaudiooutput

# This is more of a system test
CONFIG -= testcase

DEFINES += SRCDIR=\\\"$$PWD/\\\"

HEADERS += wavheader.h
SOURCES += wavheader.cpp tst_qaudiooutput.cpp

maemo*:CONFIG += insignificant_test
