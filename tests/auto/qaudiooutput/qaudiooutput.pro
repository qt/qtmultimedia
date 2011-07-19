load(qttest_p4)

# temporarily disable on mac
CONFIG += insignificant_test

QT += core multimediakit-private

# TARGET = tst_qaudiooutput
# CONFIG += testcase

DEFINES += SRCDIR=\\\"$$PWD/\\\"

HEADERS += wavheader.h
SOURCES += wavheader.cpp tst_qaudiooutput.cpp

maemo*:CONFIG += insignificant_test
