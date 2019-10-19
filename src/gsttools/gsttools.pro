TARGET = QtMultimediaGstTools
MODULE = multimediagsttools
CONFIG += internal_module

QT = core-private multimedia-private gui-private

!static:DEFINES += QT_MAKEDLL
DEFINES += GLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_26

qtConfig(alsa): \
    QMAKE_USE += alsa

QMAKE_USE += gstreamer

qtConfig(resourcepolicy): \
    QMAKE_USE += libresourceqt5

PRIVATE_HEADERS += \
    qgstreamerbushelper_p.h \
    qgstreamermessage_p.h \
    qgstutils_p.h \
    qgstvideobuffer_p.h \
    qgstreamerbufferprobe_p.h \
    qgstreamervideorendererinterface_p.h \
    qgstreameraudioinputselector_p.h \
    qgstreamervideorenderer_p.h \
    qgstreamervideoinputdevicecontrol_p.h \
    qgstcodecsinfo_p.h \
    qgstreamervideoprobecontrol_p.h \
    qgstreameraudioprobecontrol_p.h \
    qgstreamervideowindow_p.h \
    qgstreamervideooverlay_p.h \
    qgsttools_global_p.h \
    qgstreamerplayersession_p.h \
    qgstreamerplayercontrol_p.h

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
    qgstreamervideooverlay.cpp \
    qgstreamerplayersession.cpp \
    qgstreamerplayercontrol.cpp

qtHaveModule(widgets) {
    QT += multimediawidgets

    PRIVATE_HEADERS += \
        qgstreamervideowidget_p.h

    SOURCES += \
        qgstreamervideowidget.cpp
}

qtConfig(gstreamer_0_10) {
    PRIVATE_HEADERS += \
        qgstbufferpoolinterface_p.h \
        qvideosurfacegstsink_p.h \
        gstvideoconnector_p.h

    SOURCES += \
        qgstbufferpoolinterface.cpp \
        qvideosurfacegstsink.cpp \
        gstvideoconnector.c
} else {
    PRIVATE_HEADERS += \
        qgstvideorendererplugin_p.h \
        qgstvideorenderersink_p.h

    SOURCES += \
        qgstvideorendererplugin.cpp \
        qgstvideorenderersink.cpp
}

qtConfig(gstreamer_gl): QMAKE_USE += gstreamer_gl

qtConfig(gstreamer_app) {
    QMAKE_USE += gstreamer_app
    PRIVATE_HEADERS += qgstappsrc_p.h
    SOURCES += qgstappsrc.cpp
}

android {
    LIBS_PRIVATE += \
        -L$$(GSTREAMER_ROOT_ANDROID)/armv7/lib \
        -Wl,--whole-archive \
        -lgstapp-1.0 -lgstreamer-1.0 -lgstaudio-1.0 -lgsttag-1.0 -lgstvideo-1.0 -lgstbase-1.0 -lgstpbutils-1.0 \
        -lgobject-2.0 -lgmodule-2.0 -lglib-2.0 -lffi -lintl -liconv -lorc-0.4 \
        -Wl,--no-whole-archive
}

HEADERS += $$PRIVATE_HEADERS

load(qt_module)
