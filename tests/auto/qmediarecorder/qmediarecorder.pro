load(qttest_p4)

QT += multimediakit-private

# TARGET = tst_qmediarecorder
# CONFIG += testcase

include (../qmultimedia_common/mock.pri)
include (../qmultimedia_common/mockrecorder.pri)

HEADERS += tst_qmediarecorder.h
SOURCES += main.cpp tst_qmediarecorder.cpp

