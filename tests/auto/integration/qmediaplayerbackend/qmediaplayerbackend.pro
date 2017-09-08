TARGET = tst_qmediaplayerbackend

QT += multimedia-private testlib

# This is more of a system test
CONFIG += testcase


SOURCES += \
    tst_qmediaplayerbackend.cpp

TESTDATA += testdata/*

boot2qt: {
    # OGV testing is unstable with qemu
    QMAKE_CXXFLAGS += -DSKIP_OGV_TEST
}
