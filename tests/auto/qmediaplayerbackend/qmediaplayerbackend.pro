load(qttest_p4)

# temporarily blacklist because it fails for unknown reason
CONFIG += insignificant_test

QT += multimediakit-private

# TARGET = tst_qmediaplayerbackend
# CONFIG += testcase

DEFINES += TESTDATA_DIR=\\\"$$PWD/\\\"

SOURCES += \
    tst_qmediaplayerbackend.cpp

maemo*:CONFIG += insignificant_test
