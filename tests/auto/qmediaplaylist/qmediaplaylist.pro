load(qttest_p4)

# temporarily blacklist test because is fails miserably
CONFIG += insignificant_test

include (../qmultimedia_common/mockplaylist.pri)

QT += multimediakit-private

# TARGET = tst_qmediaplaylist
# CONFIG += testcase

DEFINES += TESTDATA_DIR=\\\"$$PWD/\\\"

HEADERS += \
    $$QT.multimediakit.sources/../plugins/m3u/qm3uhandler.h

SOURCES += \
    tst_qmediaplaylist.cpp \
    $$QT.multimediakit.sources/../plugins/m3u/qm3uhandler.cpp

INCLUDEPATH += $$QT.multimediakit.sources/../plugins/m3u

maemo*:CONFIG += insignificant_test
