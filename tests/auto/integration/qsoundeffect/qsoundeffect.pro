TARGET = tst_qsoundeffect

QT += core multimedia-private testlib

# This is more of a system test
CONFIG += testcase

SOURCES += tst_qsoundeffect.cpp

unix:!mac {
    !contains(QT_CONFIG, pulseaudio) {
        DEFINES += QT_MULTIMEDIA_QMEDIAPLAYER
    }
}

TESTDATA += test.wav


config_pulseaudio: CONFIG += insignificant_testcase # Crashes in QSoundEffectPrivate::sampleReady with bufferAttr == 0
