load(qttest_p4)

QT += multimediakit-private

# TARGET = tst_qmediarecorder
# CONFIG += testcase

symbian {
    HEADERS += s60common.h
    HEADERS += tst_qmediarecorder_xa.h
    SOURCES += tst_qmediarecorder_xa.cpp
    HEADERS += tst_qmediarecorder_mmf.h
    SOURCES += tst_qmediarecorder_mmf.cpp
    TARGET.CAPABILITY = UserEnvironment ReadDeviceData WriteDeviceData AllFiles
}
HEADERS += tst_qmediarecorder.h
SOURCES += main.cpp tst_qmediarecorder.cpp

