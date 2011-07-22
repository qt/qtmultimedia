load(qttest_p4)

QT += network multimediakit-private multimediakitwidgets-private

# TARGET = tst_qmediaplayer
# CONFIG += testcase

HEADERS += tst_qmediaplayer.h
SOURCES += main.cpp tst_qmediaplayer.cpp

include (../qmultimedia_common/mock.pri)
include (../qmultimedia_common/mockplayer.pri)
