# Generated from mediaplayer.pro.

#####################################################################
## QGstreamerPlayerServicePlugin Plugin:
#####################################################################

qt_internal_add_plugin(QGstreamerPlayerServicePlugin
    OUTPUT_NAME gstmediaplayer
    TYPE mediaservice
    SOURCES
        qgstreamermetadataprovider.cpp qgstreamermetadataprovider.h
        qgstreamerplayerservice.cpp qgstreamerplayerservice.h
        qgstreamerplayerserviceplugin.cpp qgstreamerplayerserviceplugin.h
        qgstreamerstreamscontrol.cpp qgstreamerstreamscontrol.h
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

#### Keys ignored in scope 1:.:.:mediaplayer.pro:<TRUE>:
# OTHER_FILES = "mediaplayer.json"

## Scopes:
#####################################################################

qt_internal_extend_target(QGstreamerPlayerServicePlugin CONDITION TARGET Qt::Widgets
    DEFINES
        HAVE_WIDGETS
    PUBLIC_LIBRARIES
        Qt::MultimediaWidgetsPrivate
        Qt::Widgets
)

qt_internal_extend_target(QGstreamerPlayerServicePlugin CONDITION QT_FEATURE_gstreamer_app
    PUBLIC_LIBRARIES
        gstreamer_app
)
