load(qttest_p4)

QT += multimediakit-private

# TARGET = tst_qcamerabackend
# CONFIG += testcase

SOURCES += tst_qcamerabackend.cpp

maemo*:CONFIG += insignificant_test
