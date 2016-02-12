
QT += core-private multimedia-private network

qtHaveModule(widgets) {
    QT += widgets multimediawidgets-private
    DEFINES += HAVE_WIDGETS
}

LIBS += -lqgsttools_p

CONFIG += link_pkgconfig

PKGCONFIG += \
    gstreamer-$$GST_VERSION \
    gstreamer-base-$$GST_VERSION \
    gstreamer-audio-$$GST_VERSION \
    gstreamer-video-$$GST_VERSION \
    gstreamer-pbutils-$$GST_VERSION

maemo*:PKGCONFIG +=gstreamer-plugins-bad-$$GST_VERSION

mir: {
    DEFINES += HAVE_MIR
}


config_resourcepolicy {
    DEFINES += HAVE_RESOURCE_POLICY
    PKGCONFIG += libresourceqt5
}

config_gstreamer_appsrc {
    PKGCONFIG += gstreamer-app-$$GST_VERSION
    DEFINES += HAVE_GST_APPSRC
    LIBS += -lgstapp-$$GST_VERSION
}

