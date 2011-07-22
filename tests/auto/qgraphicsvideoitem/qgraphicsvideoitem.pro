load(qttest_p4)

QT += multimediakit-private multimediakitwidgets-private

# TARGET = tst_qgraphicsvideoitem
# CONFIG += testcase

SOURCES += tst_qgraphicsvideoitem.cpp

maemo*:CONFIG += insignificant_test
