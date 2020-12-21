# Generated from gsttools.pro.

#####################################################################
## MultimediaGstTools Module:
#####################################################################

qt_add_module(MultimediaGstTools
    INTERNAL_MODULE
    SOURCES
        qgstcodecsinfo.cpp qgstcodecsinfo_p.h
        qgstreameraudioinputselector.cpp qgstreameraudioinputselector_p.h
        qgstreameraudioprobecontrol.cpp qgstreameraudioprobecontrol_p.h
        qgstreamerbufferprobe.cpp qgstreamerbufferprobe_p.h
        qgstreamerbushelper.cpp qgstreamerbushelper_p.h
        qgstreamermessage.cpp qgstreamermessage_p.h
        qgstreamerplayercontrol.cpp qgstreamerplayercontrol_p.h
        qgstreamerplayersession.cpp qgstreamerplayersession_p.h
        qgstreamervideoinputdevicecontrol.cpp qgstreamervideoinputdevicecontrol_p.h
        qgstreamervideooverlay.cpp qgstreamervideooverlay_p.h
        qgstreamervideoprobecontrol.cpp qgstreamervideoprobecontrol_p.h
        qgstreamervideorenderer.cpp qgstreamervideorenderer_p.h
        qgstreamervideorendererinterface.cpp qgstreamervideorendererinterface_p.h
        qgstreamervideowindow.cpp qgstreamervideowindow_p.h
        qgsttools_global_p.h
        qgstutils.cpp qgstutils_p.h
        qgstvideobuffer.cpp qgstvideobuffer_p.h
        qgstvideorendererplugin.cpp qgstvideorendererplugin_p.h
        qgstvideorenderersink.cpp qgstvideorenderersink_p.h
    DEFINES
        GLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_26
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::MultimediaPrivate
        gstreamer
)

#### Keys ignored in scope 1:.:.:gsttools.pro:<TRUE>:
# MODULE = "multimediagsttools"

## Scopes:
#####################################################################

qt_extend_target(MultimediaGstTools CONDITION NOT static
    DEFINES
        QT_MAKEDLL
)

qt_extend_target(MultimediaGstTools CONDITION QT_FEATURE_alsa
    PUBLIC_LIBRARIES
        alsa
)

qt_extend_target(MultimediaGstTools CONDITION TARGET Qt::Widgets
    SOURCES
        qgstreamervideowidget.cpp qgstreamervideowidget_p.h # special case
    PUBLIC_LIBRARIES
        Qt::MultimediaWidgets
)

#### Keys ignored in scope 4:.:.:gsttools.pro:TARGET Qt::Widgets:
# PRIVATE_HEADERS = "qgstreamervideowidget_p.h"

qt_extend_target(MultimediaGstTools CONDITION QT_FEATURE_gstreamer_gl
    PUBLIC_LIBRARIES
        gstreamer_gl
)

qt_extend_target(MultimediaGstTools CONDITION QT_FEATURE_gstreamer_app
    SOURCES
        qgstappsrc.cpp qgstappsrc_p.h # special case
    PUBLIC_LIBRARIES
        gstreamer_app
)

#### Keys ignored in scope 6:.:.:gsttools.pro:QT_FEATURE_gstreamer_app:
# PRIVATE_HEADERS = "qgstappsrc_p.h"

qt_extend_target(MultimediaGstTools CONDITION ANDROID
    LIBRARIES
        # Remove: L$ENV{GSTREAMER_ROOT_ANDROID}/armv7/lib
        # Remove: Wl,--no-whole-archive
        # Remove: Wl,--whole-archive
        WrapIconv::WrapIconv
        ffi
        glib-2.0
        gmodule-2.0
        gobject-2.0
        gstapp-1.0
        gstaudio-1.0
        gstbase-1.0
        gstpbutils-1.0
        gstreamer-1.0
        gsttag-1.0
        gstvideo-1.0
        intl
        orc-0.4
)
