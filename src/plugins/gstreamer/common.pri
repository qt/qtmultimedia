QT += core-private multimedia-private multimediagsttools-private network

qtHaveModule(widgets) {
    QT += widgets multimediawidgets-private
    DEFINES += HAVE_WIDGETS
}

QMAKE_USE += gstreamer

qtConfig(gstreamer_app): \
    QMAKE_USE += gstreamer_app

