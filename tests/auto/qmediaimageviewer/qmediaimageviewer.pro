load(qttest_p4)

QT += multimediakit-private network

# TARGET = tst_qmediaimageviewer
# CONFIG += testcase

SOURCES += tst_qmediaimageviewer.cpp

RESOURCES += \
        images.qrc

!contains(QT_CONFIG, no-jpeg):DEFINES += QTEST_HAVE_JPEG

wince* {
    !contains(QT_CONFIG, no-jpeg): DEPLOYMENT_PLUGIN += qjpeg
}
