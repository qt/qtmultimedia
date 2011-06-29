load(qttest_p4)

QT += multimediakit-private

# TARGET = tst_qvideowidget
# CONFIG += testcase

SOURCES += tst_qvideowidget.cpp

maemo*:CONFIG += insignificant_test
