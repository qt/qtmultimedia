load(qttest_p4)

QT += multimediakit-private

# TARGET = tst_qcamera
# CONFIG += testcase

SOURCES += tst_qcamera.cpp

maemo*:CONFIG += insignificant_test
