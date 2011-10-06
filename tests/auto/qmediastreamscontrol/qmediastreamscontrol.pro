load(qttest_p4)

QT += multimedia-private
CONFIG += no_private_qt_headers_warning

SOURCES += \
        tst_qmediastreamscontrol.cpp

include(../multimedia_common.pri)
