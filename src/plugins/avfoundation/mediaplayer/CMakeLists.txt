# Generated from mediaplayer.pro.

#####################################################################
## AVFMediaPlayerServicePlugin Plugin:
#####################################################################

qt_internal_add_plugin(AVFMediaPlayerServicePlugin
    OUTPUT_NAME qavfmediaplayer
    TYPE mediaservice
    SOURCES
        avfmediaplayercontrol.h avfmediaplayercontrol.mm
        avfmediaplayermetadatacontrol.h avfmediaplayermetadatacontrol.mm
        avfmediaplayerservice.h avfmediaplayerservice.mm
        avfmediaplayerserviceplugin.h avfmediaplayerserviceplugin.mm
        avfmediaplayersession.h avfmediaplayersession.mm
        avfvideooutput.h avfvideooutput.mm
        avfvideowindowcontrol.h avfvideowindowcontrol.mm
    PUBLIC_LIBRARIES
        ${FWCoreMedia}
        ${FWCoreVideo}
        ${FWMetal}
        ${FWQuartzCore}
        Qt::Core
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::Network
        Qt::OpenGL
        avfoundation
)

#### Keys ignored in scope 1:.:.:mediaplayer.pro:<TRUE>:
# OTHER_FILES = "avfmediaplayer.json"

## Scopes:
#####################################################################

qt_internal_extend_target(AVFMediaPlayerServicePlugin CONDITION TARGET Qt::Widgets
    SOURCES
        avfvideowidget.h avfvideowidget.mm
        avfvideowidgetcontrol.h avfvideowidgetcontrol.mm
    PUBLIC_LIBRARIES
        Qt::MultimediaWidgetsPrivate
)

qt_internal_extend_target(AVFMediaPlayerServicePlugin CONDITION IOS OR TVOS
    PUBLIC_LIBRARIES
        ${FWFoundation}
)

qt_internal_extend_target(AVFMediaPlayerServicePlugin CONDITION QT_FEATURE_opengl AND (IOS OR TVOS)
    SOURCES
        avfdisplaylink.h avfdisplaylink.mm
        avfvideoframerenderer_ios.h avfvideoframerenderer_ios.mm
        avfvideorenderercontrol.h avfvideorenderercontrol.mm
)

qt_internal_extend_target(AVFMediaPlayerServicePlugin CONDITION NOT IOS AND NOT TVOS
    PUBLIC_LIBRARIES
        ${FWAppKit}
)

qt_internal_extend_target(AVFMediaPlayerServicePlugin CONDITION QT_FEATURE_opengl AND NOT IOS AND NOT TVOS
    SOURCES
        avfdisplaylink.h avfdisplaylink.mm
        avfvideoframerenderer.h avfvideoframerenderer.mm
        avfvideorenderercontrol.h avfvideorenderercontrol.mm
)
