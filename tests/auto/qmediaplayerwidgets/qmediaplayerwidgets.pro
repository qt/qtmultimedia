load(qttest_p4)

QT += network multimediakit-private multimediakitwidgets-private
CONFIG += no_private_qt_headers_warning

HEADERS += tst_qmediaplayerwidgets.h
SOURCES += main.cpp tst_qmediaplayerwidgets.cpp

include (../qmultimedia_common/mock.pri)
include (../qmultimedia_common/mockplayer.pri)
