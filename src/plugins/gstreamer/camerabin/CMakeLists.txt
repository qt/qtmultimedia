# Generated from camerabin.pro.

#####################################################################
## CameraBinServicePlugin Plugin:
#####################################################################

qt_internal_add_plugin(CameraBinServicePlugin
    OUTPUT_NAME gstcamerabin
    TYPE mediaservice
    SOURCES
        camerabinaudioencoder.cpp camerabinaudioencoder.h
        camerabincapturebufferformat.cpp camerabincapturebufferformat.h
        camerabincapturedestination.cpp camerabincapturedestination.h
        camerabincontainer.cpp camerabincontainer.h
        camerabincontrol.cpp camerabincontrol.h
        camerabinimagecapture.cpp camerabinimagecapture.h
        camerabinimageencoder.cpp camerabinimageencoder.h
        camerabinimageprocessing.cpp camerabinimageprocessing.h
        camerabininfocontrol.cpp camerabininfocontrol.h
        camerabinmetadata.cpp camerabinmetadata.h
        camerabinrecorder.cpp camerabinrecorder.h
        camerabinservice.cpp camerabinservice.h
        camerabinserviceplugin.cpp camerabinserviceplugin.h
        camerabinsession.cpp camerabinsession.h
        camerabinvideoencoder.cpp camerabinvideoencoder.h
        camerabinviewfindersettings.cpp camerabinviewfindersettings.h
        camerabinviewfindersettings2.cpp camerabinviewfindersettings2.h
        camerabinzoom.cpp camerabinzoom.h
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}
        /src/multimedia
        camerabin
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::Network
        gstreamer
        multimediagsttoolsPrivate
)

#### Keys ignored in scope 1:.:.:camerabin.pro:<TRUE>:
# OTHER_FILES = "camerabin.json"

## Scopes:
#####################################################################

qt_internal_extend_target(CameraBinServicePlugin CONDITION QT_FEATURE_gstreamer_photography
    SOURCES
        camerabinexposure.cpp camerabinexposure.h
        camerabinflash.cpp camerabinflash.h
        camerabinfocus.cpp camerabinfocus.h
        camerabinlocks.cpp camerabinlocks.h
    DEFINES
        GST_USE_UNSTABLE_API
    PUBLIC_LIBRARIES
        gstreamer_photography
)

qt_internal_extend_target(CameraBinServicePlugin CONDITION QT_FEATURE_linux_v4l
    SOURCES
        camerabinv4limageprocessing.cpp camerabinv4limageprocessing.h
)

qt_internal_extend_target(CameraBinServicePlugin CONDITION TARGET Qt::Widgets
    DEFINES
        HAVE_WIDGETS
    PUBLIC_LIBRARIES
        Qt::MultimediaWidgetsPrivate
        Qt::Widgets
)

qt_internal_extend_target(CameraBinServicePlugin CONDITION QT_FEATURE_gstreamer_app
    PUBLIC_LIBRARIES
        gstreamer_app
)
