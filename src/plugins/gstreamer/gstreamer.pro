
load(qt_module)

TARGET = qgstengine
QT += multimediakit-private network
PLUGIN_TYPE=mediaservice

load(qt_plugin)
DESTDIR = $$QT.multimediakit.plugins/$${PLUGIN_TYPE}

unix:!maemo*:contains(QT_CONFIG, alsa) {
DEFINES += HAVE_ALSA
LIBS += \
    -lasound
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
contains(config_test_gstreamer_appsrc, yes): PKGCONFIG += gstreamer-app-0.10

maemo6 {
    HEADERS += camerabuttonlistener_meego.h
    SOURCES += camerabuttonlistener_meego.cpp

    PKGCONFIG += qmsystem2 libresourceqt1

    isEqual(QT_ARCH,armv6) {
        HEADERS += qgstreamergltexturerenderer.h
        SOURCES += qgstreamergltexturerenderer.cpp
        QT += opengl
        LIBS += -lEGL -lgstmeegointerfaces-0.10
    }
}

# Input
HEADERS += \
    qgstreamermessage.h \
    qgstreamerbushelper.h \
    qgstreamervideorendererinterface.h \
    qgstreamerserviceplugin.h \
    qgstreameraudioinputendpointselector.h \
    qgstreamervideorenderer.h \
    qgstvideobuffer.h \
    qvideosurfacegstsink.h \
    qgstreamervideoinputdevicecontrol.h \
    gstvideoconnector.h \
    qabstractgstbufferpool.h \
    qgstcodecsinfo.h \
    qgstutils.h

SOURCES += \
    qgstreamermessage.cpp \
    qgstreamerbushelper.cpp \
    qgstreamervideorendererinterface.cpp \
    qgstreamerserviceplugin.cpp \
    qgstreameraudioinputendpointselector.cpp \
    qgstreamervideorenderer.cpp \
    qgstvideobuffer.cpp \
    qvideosurfacegstsink.cpp \
    qgstreamervideoinputdevicecontrol.cpp \
    qgstcodecsinfo.cpp \
    gstvideoconnector.c \
    qgstutils.cpp


!win32:!contains(QT_CONFIG,embedded):!mac:!simulator:!contains(QT_CONFIG, qpa) {
    LIBS += -lXv -lX11 -lXext

    HEADERS += \
        qgstreamervideooverlay.h \
        qgstreamervideowindow.h \
        qgstreamervideowidget.h \
        qx11videosurface.h \
        qgstxvimagebuffer.h

    SOURCES += \
        qgstreamervideooverlay.cpp \
        qgstreamervideowindow.cpp \
        qgstreamervideowidget.cpp \
        qx11videosurface.cpp \
        qgstxvimagebuffer.cpp
}
include(mediaplayer/mediaplayer.pri)
include(mediacapture/mediacapture.pri)

contains(config_test_gstreamer_photography, yes) {
    include(camerabin/camerabin.pri)
}

target.path += $$[QT_INSTALL_PLUGINS]/$${PLUGIN_TYPE}
INSTALLS += target
