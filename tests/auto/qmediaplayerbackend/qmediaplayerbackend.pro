load(qttest_p4)

QT += multimediakit-private
CONFIG += no_private_qt_headers_warning

# This is more of a system test
CONFIG -= testcase

DEFINES += TESTDATA_DIR=\\\"$$PWD/\\\"

SOURCES += \
    tst_qmediaplayerbackend.cpp

maemo*:CONFIG += insignificant_test
