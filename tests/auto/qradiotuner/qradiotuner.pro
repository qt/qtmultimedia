load(qttest_p4)

QT += multimediakit-private

# TARGET = tst_qradiotuner
# CONFIG += testcase

HEADERS += tst_qradiotuner.h
SOURCES += main.cpp tst_qradiotuner.cpp

include (../qmultimedia_common/mock.pri)
include (../qmultimedia_common/mockradio.pri)
