load(qttest_p4)

QT += multimediakit-private multimediakitwidgets-private
CONFIG += no_private_qt_headers_warning

SOURCES += \
        tst_qmediastreamscontrol.cpp

include(../multimedia_common.pri)
