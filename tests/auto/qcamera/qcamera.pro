load(qttest_p4)

QT += multimediakit-private

include (../qmultimedia_common/mock.pri)
include (../qmultimedia_common/mockcamera.pri)

# TARGET = tst_qcamera
# CONFIG += testcase

SOURCES += tst_qcamera.cpp

maemo*:CONFIG += insignificant_test
