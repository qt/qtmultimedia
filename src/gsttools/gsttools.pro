TEMPLATE = lib

TARGET = qgsttools_p
QPRO_PWD = $$PWD

QT = core-private multimedia-private gui-private

!static:DEFINES += QT_MAKEDLL
DEFINES += GLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_26

unix:!maemo*:contains(QT_CONFIG, alsa) {
DEFINES += HAVE_ALSA
LIBS_PRIVATE += \
    -lasound
}

CONFIG += link_pkgconfig

PKGCONFIG += \
    gstreamer-$$GST_VERSION \
    gstreamer-base-$$GST_VERSION \
    gstreamer-audio-$$GST_VERSION \
    gstreamer-video-$$GST_VERSION \
    gstreamer-pbutils-$$GST_VERSION

equals(GST_VERSION,"0.10") {
    PKGCONFIG_PRIVATE += gstreamer-interfaces-0.10
    maemo*: PKGCONFIG_PRIVATE +=gstreamer-plugins-bad-0.10
}

config_resourcepolicy {
    DEFINES += HAVE_RESOURCE_POLICY
    PKGCONFIG_PRIVATE += libresourceqt5
}

# Header files must go inside source directory of a module
# to be installed by syncqt.
INCLUDEPATH += ../multimedia/gsttools_headers/
INCLUDEPATH += ../plugins/gstreamer/mediaplayer/
VPATH += ../multimedia/gsttools_headers/

PRIVATE_HEADERS += \
    qgstreamerbushelper_p.h \
    qgstreamermessage_p.h \
    qgstutils_p.h \
    qgstvideobuffer_p.h \
    qvideosurfacegstsink_p.h \
    qgstreamerbufferprobe_p.h \
    qgstreamervideorendererinterface_p.h \
    qgstreameraudioinputselector_p.h \
    qgstreamervideorenderer_p.h \
    qgstreamervideoinputdevicecontrol_p.h \
    qgstcodecsinfo_p.h \
    qgstreamervideoprobecontrol_p.h \
    qgstreameraudioprobecontrol_p.h \
    qgstreamervideowindow_p.h \
    qgstreamervideooverlay_p.h

SOURCES += \
    qgstreamerbushelper.cpp \
    qgstreamermessage.cpp \
    qgstutils.cpp \
    qgstvideobuffer.cpp \
    qgstreamerbufferprobe.cpp \
    qgstreamervideorendererinterface.cpp \
    qgstreameraudioinputselector.cpp \
    qgstreamervideorenderer.cpp \
    qgstreamervideoinputdevicecontrol.cpp \
    qgstcodecsinfo.cpp \
    qgstreamervideoprobecontrol.cpp \
    qgstreameraudioprobecontrol.cpp \
    qgstreamervideowindow.cpp \
    qgstreamervideooverlay.cpp

qtHaveModule(widgets) {
    QT += multimediawidgets

    PRIVATE_HEADERS += \
        qgstreamervideowidget_p.h

    SOURCES += \
        qgstreamervideowidget.cpp
}

equals(GST_VERSION,"0.10") {
    PRIVATE_HEADERS += \
        qgstbufferpoolinterface_p.h \
        gstvideoconnector_p.h \

    SOURCES += \
        qgstbufferpoolinterface.cpp \
        qvideosurfacegstsink.cpp \
        gstvideoconnector.c

    maemo6 {
        PKGCONFIG_PRIVATE += qmsystem2

        contains(QT_CONFIG, opengles2):qtHaveModule(widgets) {
            PRIVATE_HEADERS += qgstreamergltexturerenderer_p.h
            SOURCES += qgstreamergltexturerenderer.cpp
            QT += opengl
            LIBS_PRIVATE += -lEGL -lgstmeegointerfaces-0.10
        }
    }
} else {
    PRIVATE_HEADERS += \
        qgstvideorendererplugin_p.h \
        qgstvideorenderersink_p.h

    SOURCES += \
        qgstvideorendererplugin.cpp \
        qgstvideorenderersink.cpp
}

mir: {
    contains(QT_CONFIG, opengles2):qtHaveModule(widgets) {
        PRIVATE_HEADERS += qgstreamermirtexturerenderer_p.h
        SOURCES += qgstreamermirtexturerenderer.cpp
        QT += opengl quick
        LIBS += -lEGL
    }
    DEFINES += HAVE_MIR
}

config_gstreamer_appsrc {
    PKGCONFIG_PRIVATE += gstreamer-app-$$GST_VERSION
    PRIVATE_HEADERS += qgstappsrc_p.h
    SOURCES += qgstappsrc.cpp

    DEFINES += HAVE_GST_APPSRC

    LIBS_PRIVATE += -lgstapp-$$GST_VERSION
}

config_linux_v4l: DEFINES += USE_V4L

HEADERS += $$PRIVATE_HEADERS

DESTDIR = $$QT.multimedia.libs
target.path = $$[QT_INSTALL_LIBS]

INSTALLS += target
