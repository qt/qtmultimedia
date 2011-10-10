CONFIG += testcase
TARGET = tst_qmediaimageviewer

QT += multimedia-private network testlib
CONFIG += no_private_qt_headers_warning

SOURCES += tst_qmediaimageviewer.cpp

RESOURCES += \
        images.qrc

!contains(QT_CONFIG, no-jpeg):DEFINES += QTEST_HAVE_JPEG

wince* {
    !contains(QT_CONFIG, no-jpeg): DEPLOYMENT_PLUGIN += qjpeg
}
