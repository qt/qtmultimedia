DEFINES += GLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_26

QMAKE_USE_PRIVATE += gstreamer gstreamer_app

HEADERS += \
    platform/gstreamer/qaudiointerface_gstreamer_p.h \
    platform/gstreamer/qaudiodeviceinfo_gstreamer_p.h \
    platform/gstreamer/qaudiooutput_gstreamer_p.h \
    platform/gstreamer/qaudioinput_gstreamer_p.h \
    platform/gstreamer/qaudioengine_gstreamer_p.h \
    platform/gstreamer/qgstappsrc_p.h \
    platform/gstreamer/qgstreamerbushelper_p.h \
    platform/gstreamer/qgstreamermessage_p.h \
    platform/gstreamer/qgstutils_p.h \
    platform/gstreamer/qgstvideobuffer_p.h \
    platform/gstreamer/qgstreamerbufferprobe_p.h \
    platform/gstreamer/qgstreamervideorendererinterface_p.h \
    platform/gstreamer/qgstreameraudioinputselector_p.h \
    platform/gstreamer/qgstreamervideorenderer_p.h \
    platform/gstreamer/qgstreamervideoinputdevicecontrol_p.h \
    platform/gstreamer/qgstcodecsinfo_p.h \
    platform/gstreamer/qgstreamervideoprobecontrol_p.h \
    platform/gstreamer/qgstreameraudioprobecontrol_p.h \
    platform/gstreamer/qgstreamervideowindow_p.h \
    platform/gstreamer/qgstreamervideooverlay_p.h \
    platform/gstreamer/qgstreamerplayersession_p.h \
    platform/gstreamer/qgstreamerplayercontrol_p.h \
    platform/gstreamer/qgstvideorendererplugin_p.h \
    platform/gstreamer/qgstvideorenderersink_p.h

SOURCES += \
    platform/gstreamer/qaudiointerface_gstreamer.cpp \
    platform/gstreamer/qaudiodeviceinfo_gstreamer.cpp \
    platform/gstreamer/qaudiooutput_gstreamer.cpp \
    platform/gstreamer/qaudioinput_gstreamer.cpp \
    platform/gstreamer/qaudioengine_gstreamer.cpp \
    platform/gstreamer/qgstappsrc.cpp \
    platform/gstreamer/qgstreamerbushelper.cpp \
    platform/gstreamer/qgstreamermessage.cpp \
    platform/gstreamer/qgstutils.cpp \
    platform/gstreamer/qgstvideobuffer.cpp \
    platform/gstreamer/qgstreamerbufferprobe.cpp \
    platform/gstreamer/qgstreamervideorendererinterface.cpp \
    platform/gstreamer/qgstreameraudioinputselector.cpp \
    platform/gstreamer/qgstreamervideorenderer.cpp \
    platform/gstreamer/qgstreamervideoinputdevicecontrol.cpp \
    platform/gstreamer/qgstcodecsinfo.cpp \
    platform/gstreamer/qgstreamervideoprobecontrol.cpp \
    platform/gstreamer/qgstreameraudioprobecontrol.cpp \
    platform/gstreamer/qgstreamervideowindow.cpp \
    platform/gstreamer/qgstreamervideooverlay.cpp \
    platform/gstreamer/qgstreamerplayersession.cpp \
    platform/gstreamer/qgstreamerplayercontrol.cpp \
    platform/gstreamer/qgstvideorendererplugin.cpp \
    platform/gstreamer/qgstvideorenderersink.cpp

qtConfig(gstreamer_gl): QMAKE_USE += gstreamer_gl

android {
    LIBS_PRIVATE += \
        -L$$(GSTREAMER_ROOT_ANDROID)/armv7/lib \
        -Wl,--whole-archive \
        -lgstapp-1.0 -lgstreamer-1.0 -lgstaudio-1.0 -lgsttag-1.0 -lgstvideo-1.0 -lgstbase-1.0 -lgstpbutils-1.0 \
        -lgobject-2.0 -lgmodule-2.0 -lglib-2.0 -lffi -lintl -liconv -lorc-0.4 \
        -Wl,--no-whole-archive
}

