load(qttest_p4)

QT += core multimediakit-private

# TARGET = tst_qaudiooutput
# CONFIG += testcase

wince*: {
    DEFINES += SRCDIR=\\\"\\\"
    QT += gui
} else {
    !symbian:DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

HEADERS += wavheader.h
SOURCES += wavheader.cpp tst_qaudiooutput.cpp

maemo*:CONFIG += insignificant_test
