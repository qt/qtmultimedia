load(qttest_p4)

QT += multimediakit-private multimediakitwidgets-private

# TARGET = tst_qvideowidget
# CONFIG += testcase

SOURCES += tst_qvideowidget.cpp

# QPA seems to break some assumptions
qpa:CONFIG += insignificant_test
