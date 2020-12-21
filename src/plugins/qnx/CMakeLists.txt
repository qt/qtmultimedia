# Generated from qnx.pro.

#####################################################################
## NeutrinoServicePlugin Plugin:
#####################################################################

qt_internal_add_plugin(NeutrinoServicePlugin
    OUTPUT_NAME qtmedia_qnx
    TYPE mediaservice
    SOURCES
        common/windowgrabber.cpp common/windowgrabber.h
        mediaplayer/mmrenderermediaplayercontrol.cpp mediaplayer/mmrenderermediaplayercontrol.h
        mediaplayer/mmrenderermediaplayerservice.cpp mediaplayer/mmrenderermediaplayerservice.h
        mediaplayer/mmrenderermetadata.cpp mediaplayer/mmrenderermetadata.h
        mediaplayer/mmrenderermetadatareadercontrol.cpp mediaplayer/mmrenderermetadatareadercontrol.h
        mediaplayer/mmrendererplayervideorenderercontrol.cpp mediaplayer/mmrendererplayervideorenderercontrol.h
        mediaplayer/mmrendererutil.cpp mediaplayer/mmrendererutil.h
        mediaplayer/mmrenderervideowindowcontrol.cpp mediaplayer/mmrenderervideowindowcontrol.h
        mediaplayer/mmreventmediaplayercontrol.cpp mediaplayer/mmreventmediaplayercontrol.h
        mediaplayer/mmreventthread.cpp mediaplayer/mmreventthread.h
        neutrinoserviceplugin.cpp neutrinoserviceplugin.h
    INCLUDE_DIRECTORIES
        common
        mediaplayer
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::MultimediaPrivate
        mmrenderer
        screen
)

#### Keys ignored in scope 1:.:.:qnx.pro:<TRUE>:
# OTHER_FILES = "neutrino_mediaservice.json"
