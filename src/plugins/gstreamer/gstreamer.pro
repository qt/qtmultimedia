
load(qt_module)

TARGET = qgstengine
QT += multimedia-private network multimediawidgets-private
PLUGIN_TYPE=mediaservice

load(qt_plugin)
DESTDIR = $$QT.multimedia.plugins/$${PLUGIN_TYPE}

LIBS += -lqgsttools_p

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

contains(config_test_resourcepolicy, yes) {
    DEFINES += HAVE_RESOURCE_POLICY
    PKGCONFIG += libresourceqt1
}

maemo6 {
    HEADERS += camerabuttonlistener_meego.h
    SOURCES += camerabuttonlistener_meego.cpp

    PKGCONFIG += qmsystem2

    isEqual(QT_ARCH,armv6) {
        HEADERS += qgstreamergltexturerenderer.h
        SOURCES += qgstreamergltexturerenderer.cpp
        QT += opengl
        LIBS += -lEGL -lgstmeegointerfaces-0.10
    }
}

# Input
HEADERS += \
    qgstreamervideorendererinterface.h \
    qgstreamerserviceplugin.h \
    qgstreameraudioinputendpointselector.h \
    qgstreamervideorenderer.h \
    qgstreamervideoinputdevicecontrol.h \
    gstvideoconnector.h \
    qgstcodecsinfo.h \

SOURCES += \
    qgstreamervideorendererinterface.cpp \
    qgstreamerserviceplugin.cpp \
    qgstreameraudioinputendpointselector.cpp \
    qgstreamervideorenderer.cpp \
    qgstreamervideoinputdevicecontrol.cpp \
    qgstcodecsinfo.cpp \
    gstvideoconnector.c \


contains(config_test_xvideo, yes) {
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

contains(config_test_gstreamer_photography, yes) {
    include(camerabin/camerabin.pri)
}

target.path += $$[QT_INSTALL_PLUGINS]/$${PLUGIN_TYPE}
INSTALLS += target
