load(qttest_p4)

QT += multimedia-private
CONFIG += no_private_qt_headers_warning

HEADERS += tst_qradiotuner.h
SOURCES += main.cpp tst_qradiotuner.cpp

include (../qmultimedia_common/mock.pri)
include (../qmultimedia_common/mockradio.pri)
