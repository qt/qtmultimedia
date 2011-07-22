load(qttest_p4)

QT += multimediakit-private multimediakitwidgets-private

# TARGET = tst_qcamerabackend

# This is more of a system test
CONFIG -= testcase

SOURCES += tst_qcamerabackend.cpp

maemo*:CONFIG += insignificant_test
