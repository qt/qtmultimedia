load(qttest_p4)

QT += multimediakit-private
CONFIG += no_private_qt_headers_warning

SOURCES += tst_qmediacontainercontrol.cpp

include (../qmultimedia_common/mockcontainer.pri)

