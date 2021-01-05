DEFINES += GLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_26

qtConfig(alsa): \
    QMAKE_USE += alsa

QMAKE_USE += gstreamer

HEADERS += \
    gstreamer/qgstreamerbushelper_p.h \
    gstreamer/qgstreamermessage_p.h \
    gstreamer/qgstutils_p.h \
    gstreamer/qgstvideobuffer_p.h \
    gstreamer/qgstreamerbufferprobe_p.h \
    gstreamer/qgstreamervideorendererinterface_p.h \
    gstreamer/qgstreameraudioinputselector_p.h \
    gstreamer/qgstreamervideorenderer_p.h \
    gstreamer/qgstreamervideoinputdevicecontrol_p.h \
    gstreamer/qgstcodecsinfo_p.h \
    gstreamer/qgstreamervideoprobecontrol_p.h \
    gstreamer/qgstreameraudioprobecontrol_p.h \
    gstreamer/qgstreamervideowindow_p.h \
    gstreamer/qgstreamervideooverlay_p.h \
    gstreamer/qgstreamerplayersession_p.h \
    gstreamer/qgstreamerplayercontrol_p.h \
    gstreamer/qgstvideorendererplugin_p.h \
    gstreamer/qgstvideorenderersink_p.h

SOURCES += \
    gstreamer/qgstreamerbushelper.cpp \
    gstreamer/qgstreamermessage.cpp \
    gstreamer/qgstutils.cpp \
    gstreamer/qgstvideobuffer.cpp \
    gstreamer/qgstreamerbufferprobe.cpp \
    gstreamer/qgstreamervideorendererinterface.cpp \
    gstreamer/qgstreameraudioinputselector.cpp \
    gstreamer/qgstreamervideorenderer.cpp \
    gstreamer/qgstreamervideoinputdevicecontrol.cpp \
    gstreamer/qgstcodecsinfo.cpp \
    gstreamer/qgstreamervideoprobecontrol.cpp \
    gstreamer/qgstreameraudioprobecontrol.cpp \
    gstreamer/qgstreamervideowindow.cpp \
    gstreamer/qgstreamervideooverlay.cpp \
    gstreamer/qgstreamerplayersession.cpp \
    gstreamer/qgstreamerplayercontrol.cpp \
    gstreamer/qgstvideorendererplugin.cpp \
    gstreamer/qgstvideorenderersink.cpp

qtConfig(gstreamer_gl): QMAKE_USE += gstreamer_gl

qtConfig(gstreamer_app) {
    QMAKE_USE += gstreamer_app
    PRIVATE_HEADERS += gstreamer/qgstappsrc_p.h
    SOURCES += gstreamer/qgstappsrc.cpp
}

android {
    LIBS_PRIVATE += \
        -L$$(GSTREAMER_ROOT_ANDROID)/armv7/lib \
        -Wl,--whole-archive \
        -lgstapp-1.0 -lgstreamer-1.0 -lgstaudio-1.0 -lgsttag-1.0 -lgstvideo-1.0 -lgstbase-1.0 -lgstpbutils-1.0 \
        -lgobject-2.0 -lgmodule-2.0 -lglib-2.0 -lffi -lintl -liconv -lorc-0.4 \
        -Wl,--no-whole-archive
}

