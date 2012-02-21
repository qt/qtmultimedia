
load(qt_module)

TARGET = qgstengine
QT += multimedia-private network
CONFIG += no_private_qt_headers_warning

!isEmpty(QT.widgets.name) {
    QT += widgets multimediawidgets-private
    DEFINES += HAVE_WIDGETS
}

PLUGIN_TYPE=mediaservice

load(qt_plugin)
DESTDIR = $$QT.multimedia.plugins/$${PLUGIN_TYPE}

LIBS += -lqgsttools_p

unix:!maemo*:contains(QT_CONFIG, alsa) {
    DEFINES += HAVE_ALSA
    LIBS += -lasound
}

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

maemo6 {
    HEADERS += camerabuttonlistener_meego.h
    SOURCES += camerabuttonlistener_meego.cpp

    PKGCONFIG += qmsystem2

    isEqual(QT_ARCH,armv6):!isEmpty(QT.widgets.name) {
        HEADERS += qgstreamergltexturerenderer.h
        SOURCES += qgstreamergltexturerenderer.cpp
        QT += opengl
        LIBS += -lEGL -lgstmeegointerfaces-0.10
    }
}

HEADERS += \
    qgstreamervideorendererinterface.h \
    qgstreamerserviceplugin.h \
    qgstreameraudioinputendpointselector.h \
    qgstreamervideorenderer.h \
    qgstreamervideoinputdevicecontrol.h \
    gstvideoconnector.h \
    qgstcodecsinfo.h \
    qgstreamervideoprobecontrol.h \
    qgstreameraudioprobecontrol.h \

SOURCES += \
    qgstreamervideorendererinterface.cpp \
    qgstreamerserviceplugin.cpp \
    qgstreameraudioinputendpointselector.cpp \
    qgstreamervideorenderer.cpp \
    qgstreamervideoinputdevicecontrol.cpp \
    qgstcodecsinfo.cpp \
    gstvideoconnector.c \
    qgstreamervideoprobecontrol.cpp \
    qgstreameraudioprobecontrol.cpp \


contains(config_test_xvideo, yes):!isEmpty(QT.widgets.name): {
    DEFINES += HAVE_XVIDEO

    LIBS += -lXv -lX11 -lXext

    HEADERS += \
        qgstreamervideooverlay.h \
        qgstreamervideowindow.h \
        qgstreamervideowidget.h \
        qx11videosurface.h \

    SOURCES += \
        qgstreamervideooverlay.cpp \
        qgstreamervideowindow.cpp \
        qgstreamervideowidget.cpp \
        qx11videosurface.cpp \
}
include(mediaplayer/mediaplayer.pri)
include(mediacapture/mediacapture.pri)
include(audiodecoder/audiodecoder.pri)

contains(config_test_gstreamer_appsrc, yes) {
    PKGCONFIG += gstreamer-app-0.10
    HEADERS += $$PWD/qgstappsrc.h
    SOURCES += $$PWD/qgstappsrc.cpp

    DEFINES += HAVE_GST_APPSRC

    LIBS += -lgstapp-0.10
}


#Camerabin2 based camera backend is untested and currently disabled
#contains(config_test_gstreamer_photography, yes) {
#    include(camerabin/camerabin.pri)
#}

target.path += $$[QT_INSTALL_PLUGINS]/$${PLUGIN_TYPE}
INSTALLS += target
