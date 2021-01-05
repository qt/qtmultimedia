# Generated from audiodecoder.pro.

#####################################################################
## QGstreamerAudioDecoderServicePlugin Plugin:
#####################################################################

qt_internal_add_plugin(QGstreamerAudioDecoderServicePlugin
    OUTPUT_NAME gstaudiodecoder
    TYPE mediaservice
    SOURCES
        qgstreameraudiodecodercontrol.cpp qgstreameraudiodecodercontrol.h
        qgstreameraudiodecoderservice.cpp qgstreameraudiodecoderservice.h
        qgstreameraudiodecoderserviceplugin.cpp qgstreameraudiodecoderserviceplugin.h
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::Network
        gstreamer
        Qt::MultimediaPrivate
)

#### Keys ignored in scope 1:.:.:audiodecoder.pro:<TRUE>:
# OTHER_FILES = "audiodecoder.json"

## Scopes:
#####################################################################

qt_internal_extend_target(QGstreamerAudioDecoderServicePlugin CONDITION TARGET Qt::Widgets
    DEFINES
        HAVE_WIDGETS
    PUBLIC_LIBRARIES
        Qt::MultimediaWidgetsPrivate
        Qt::Widgets
)

qt_internal_extend_target(QGstreamerAudioDecoderServicePlugin CONDITION QT_FEATURE_gstreamer_app
    PUBLIC_LIBRARIES
        gstreamer_app
)
