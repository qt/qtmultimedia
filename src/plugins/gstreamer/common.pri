
QT += multimedia-private network
CONFIG += no_private_qt_headers_warning

!isEmpty(QT.widgets.name) {
    QT += widgets multimediawidgets-private
    DEFINES += HAVE_WIDGETS
}

LIBS += -lqgsttools_p

CONFIG += link_pkgconfig

PKGCONFIG += \
    gstreamer-0.10 \
    gstreamer-base-0.10 \
    gstreamer-interfaces-0.10 \
    gstreamer-audio-0.10 \
    gstreamer-video-0.10 \
    gstreamer-pbutils-0.10

maemo*:PKGCONFIG +=gstreamer-plugins-bad-0.10

contains(config_test_resourcepolicy, yes) {
    DEFINES += HAVE_RESOURCE_POLICY
    PKGCONFIG += libresourceqt1
}

contains(config_test_xvideo, yes):!isEmpty(QT.widgets.name): {
    DEFINES += HAVE_XVIDEO
    LIBS += -lXv -lX11 -lXext
}

contains(config_test_gstreamer_appsrc, yes) {
    PKGCONFIG += gstreamer-app-0.10
    DEFINES += HAVE_GST_APPSRC
    LIBS += -lgstapp-0.10
}

