load(qttest_p4)

SOURCES += tst_qaudioinput.cpp

QT = core multimedia

CONFIG += insignificant_test    # QTBUG-19537

wince* {
    deploy.files += 4.wav
    DEPLOYMENT += deploy
    DEFINES += SRCDIR=\\\"\\\"
    QT += gui
} else {
    !symbian:DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

