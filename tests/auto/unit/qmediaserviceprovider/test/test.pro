CONFIG += testcase
TARGET = ../tst_qmediaserviceprovider

QT += multimedia-private testlib
CONFIG += no_private_qt_headers_warning

SOURCES += ../tst_qmediaserviceprovider.cpp

win32 {
    CONFIG(debug, debug|release) {
        TARGET = ../../debug/tst_qmediaserviceprovider
    } else {
        TARGET = ../../release/tst_qmediaserviceprovider
    }
}

mac: CONFIG -= app_bundle

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
