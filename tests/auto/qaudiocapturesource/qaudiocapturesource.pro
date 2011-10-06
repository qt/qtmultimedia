load(qttest_p4)

QT += multimedia-private
CONFIG += no_private_qt_headers_warning

SOURCES += tst_qaudiocapturesource.cpp

include (../qmultimedia_common/mockrecorder.pri)
include (../qmultimedia_common/mock.pri)

