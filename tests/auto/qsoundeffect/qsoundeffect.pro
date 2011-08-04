load(qttest_p4)

QT += core declarative multimediakit-private

# This is more of a system test
CONFIG -= testcase

SOURCES += tst_qsoundeffect.cpp

DEFINES += SRCDIR=\\\"$$PWD/\\\"

unix:!mac {
    !contains(QT_CONFIG, pulseaudio) {
        DEFINES += QT_MULTIMEDIA_QMEDIAPLAYER
    }
}

maemo*:CONFIG += insignificant_test
