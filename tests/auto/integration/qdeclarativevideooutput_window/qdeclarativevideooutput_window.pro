TARGET = tst_qdeclarativevideooutput_window

QT += multimedia-private qml testlib quick
CONFIG += no_private_qt_headers_warning
CONFIG += testcase

OTHER_FILES += \
        ../../../../src/imports/multimedia/qdeclarativevideooutput_p.h

SOURCES += \
        tst_qdeclarativevideooutput_window.cpp

INCLUDEPATH += ../../../../src/imports/multimedia
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

win32:contains(QT_CONFIG, angle): CONFIG += insignificant_test # QTBUG-28541
