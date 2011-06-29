load(qttest_p4)

QT += multimediakit-private

# TARGET = tst_qradiotuner
# CONFIG += testcase

symbian {
        HEADERS += tst_qradiotuner_xa.h
        SOURCES += tst_qradiotuner_xa.cpp
        TARGET.CAPABILITY = ALL -TCB
        HEADERS += tst_qradiotuner_s60.h
        SOURCES += tst_qradiotuner_s60.cpp
    
}

HEADERS += tst_qradiotuner.h
SOURCES += main.cpp tst_qradiotuner.cpp

