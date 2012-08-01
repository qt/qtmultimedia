CONFIG += testcase
TARGET = tst_qvideoencodersettingscontrol

QT += multimedia-private testlib
CONFIG += no_private_qt_headers_warning

SOURCES += \
    tst_qvideoencodersettingscontrol.cpp

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
