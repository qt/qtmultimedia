# Generated from wmf.pro.

#####################################################################
## WMFServicePlugin Plugin:
#####################################################################

qt_internal_add_plugin(WMFServicePlugin
    OUTPUT_NAME wmfengine
    TYPE mediaservice
    SOURCES
        ../common/evr/evrcustompresenter.cpp ../common/evr/evrcustompresenter.h
        ../common/evr/evrd3dpresentengine.cpp ../common/evr/evrd3dpresentengine.h
        ../common/evr/evrdefs.cpp ../common/evr/evrdefs.h
        ../common/evr/evrhelpers.cpp ../common/evr/evrhelpers.h
        ../common/evr/evrvideowindowcontrol.cpp ../common/evr/evrvideowindowcontrol.h
        decoder/mfaudiodecodercontrol.cpp decoder/mfaudiodecodercontrol.h
        decoder/mfdecoderservice.cpp decoder/mfdecoderservice.h
        decoder/mfdecodersourcereader.cpp decoder/mfdecodersourcereader.h
        mfstream.cpp mfstream.h
        player/mfactivate.cpp player/mfactivate.h
        player/mfaudioendpointcontrol.cpp player/mfaudioendpointcontrol.h
        player/mfaudioprobecontrol.cpp player/mfaudioprobecontrol.h
        player/mfevrvideowindowcontrol.cpp player/mfevrvideowindowcontrol.h
        player/mfmetadatacontrol.cpp player/mfmetadatacontrol.h
        player/mfplayercontrol.cpp player/mfplayercontrol.h
        player/mfplayerservice.cpp player/mfplayerservice.h
        player/mfplayersession.cpp player/mfplayersession.h
        player/mftvideo.cpp player/mftvideo.h
        player/mfvideoprobecontrol.cpp player/mfvideoprobecontrol.h
        player/mfvideorenderercontrol.cpp player/mfvideorenderercontrol.h
        player/samplegrabber.cpp player/samplegrabber.h
        sourceresolver.cpp sourceresolver.h
        wmfserviceplugin.cpp wmfserviceplugin.h
    INCLUDE_DIRECTORIES
        .
        ../common/evr
        decoder
        player
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::GuiPrivate
        Qt::MultimediaPrivate
        Qt::Network
        d3d9
        dxva2
        evr
        gdi32
        mf
        mfplat
        mfreadwrite
        mfuuid
        user32
        winmm
        wmcodecdspuuid
        wmf
)

#### Keys ignored in scope 1:.:.:wmf.pro:<TRUE>:
# OTHER_FILES = "wmf.json"

## Scopes:
#####################################################################

qt_internal_extend_target(WMFServicePlugin CONDITION WIN32 AND NOT TARGET Qt::OpenGL
    LIBRARIES
        gdi32
        user32
)

qt_internal_extend_target(WMFServicePlugin CONDITION TARGET Qt::Widgets
    PUBLIC_LIBRARIES
        Qt::Widgets
)
