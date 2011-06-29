load(qttest_p4)

QT += multimediakit-private

# TARGET = tst_qcamera
# CONFIG += testcase

SOURCES += tst_qcamera.cpp

symbian { 
    TARGET.CAPABILITY = ALL -TCB 
    TARGET.EPOCHEAPSIZE = 0x20000 0x3000000 
}

maemo*:CONFIG += insignificant_test
