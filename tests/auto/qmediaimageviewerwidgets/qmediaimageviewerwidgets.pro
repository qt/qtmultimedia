load(qttest_p4)

QT += multimediakit-private multimediakitwidgets-private network
CONFIG += no_private_qt_headers_warning

SOURCES += tst_qmediaimageviewerwidgets.cpp

RESOURCES += \
        images.qrc

!contains(QT_CONFIG, no-jpeg):DEFINES += QTEST_HAVE_JPEG

wince* {
    !contains(QT_CONFIG, no-jpeg): DEPLOYMENT_PLUGIN += qjpeg
}
