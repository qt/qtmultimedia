load(qttest_p4)

QT += multimediakit-private

# TARGET = tst_qgraphicsvideoitem
# CONFIG += testcase

SOURCES += tst_qgraphicsvideoitem.cpp

symbian: TARGET.CAPABILITY = ReadDeviceData WriteDeviceData

maemo*:CONFIG += insignificant_test
