load(qttest_p4)

# temporarily blacklist test because is fails miserably
CONFIG += insignificant_test

include (../qmultimedia_common/mockplaylist.pri)

QT += multimedia-private
CONFIG += no_private_qt_headers_warning

DEFINES += TESTDATA_DIR=\\\"$$PWD/\\\"

HEADERS += \
    $$QT.multimedia.sources/../plugins/m3u/qm3uhandler.h

SOURCES += \
    tst_qmediaplaylist.cpp \
    $$QT.multimedia.sources/../plugins/m3u/qm3uhandler.cpp

INCLUDEPATH += $$QT.multimedia.sources/../plugins/m3u

maemo*:CONFIG += insignificant_test
