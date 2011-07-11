load(qttest_p4)

QT += core declarative multimediakit-private

# TARGET = tst_qsoundeffect

SOURCES += tst_qsoundeffect.cpp

wince*|symbian {
    deploy.files = test.wav
    DEPLOYMENT = deploy
    DEFINES += QT_QSOUNDEFFECT_USEAPPLICATIONPATH
} else:maemo* {
    DEFINES += QT_QSOUNDEFFECT_USEAPPLICATIONPATH
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

unix:!mac:!symbian {
    !contains(QT_CONFIG, pulseaudio) {
        DEFINES += QT_MULTIMEDIA_QMEDIAPLAYER
    }
}

maemo*:CONFIG += insignificant_test
