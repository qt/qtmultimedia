load(qttest_p4)

QT += multimediakit-private

# TARGET = tst_qmediaplaylist
# CONFIG += testcase

symbian*: {
    PLAYLIST_TESTDATA.sources = testdata/*
    PLAYLIST_TESTDATA.path = testdata
    DEPLOYMENT += PLAYLIST_TESTDATA
}

wince* {
    DEFINES+= TESTDATA_DIR=\\\"./\\\"
}else:!symbian {
    DEFINES += TESTDATA_DIR=\\\"$$PWD/\\\"
}

HEADERS += \
    $$QT.multimediakit.sources/../plugins/m3u/qm3uhandler.h

SOURCES += \
    tst_qmediaplaylist.cpp \
    $$QT.multimediakit.sources/../plugins/m3u/qm3uhandler.cpp

INCLUDEPATH += $$QT.multimediakit.sources/../plugins/m3u

maemo*:CONFIG += insignificant_test
