load(qttest_p4)

QT += multimediakit-private multimediakitwidgets-private

# TARGET = tst_qgraphicsvideoitem
# CONFIG += testcase

SOURCES += tst_qgraphicsvideoitem.cpp

# QPA minimal crashes with this test in QBackingStore
qpa:CONFIG += insignificant_test
