# Generated from mediacapture.pro.

#####################################################################
## QGstreamerCaptureServicePlugin Plugin:
#####################################################################

qt_internal_add_plugin(QGstreamerCaptureServicePlugin
    OUTPUT_NAME gstmediacapture
    TYPE mediaservice
    SOURCES
        qgstreameraudioencode.cpp qgstreameraudioencode.h
        qgstreamercameracontrol.cpp qgstreamercameracontrol.h
        qgstreamercapturemetadatacontrol.cpp qgstreamercapturemetadatacontrol.h
        qgstreamercaptureservice.cpp qgstreamercaptureservice.h
        qgstreamercaptureserviceplugin.cpp qgstreamercaptureserviceplugin.h
        qgstreamercapturesession.cpp qgstreamercapturesession.h
        qgstreamerimagecapturecontrol.cpp qgstreamerimagecapturecontrol.h
        qgstreamerimageencode.cpp qgstreamerimageencode.h
        qgstreamermediacontainercontrol.cpp qgstreamermediacontainercontrol.h
        qgstreamerrecordercontrol.cpp qgstreamerrecordercontrol.h
        qgstreamervideoencode.cpp qgstreamervideoencode.h
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::Network
        gstreamer
        multimediagsttoolsPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(QGstreamerCaptureServicePlugin CONDITION QT_FEATURE_linux_v4l AND use_gstreamer_camera
    SOURCES
        qgstreamerv4l2input.cpp qgstreamerv4l2input.h
    DEFINES
        USE_GSTREAMER_CAMERA
)

#### Keys ignored in scope 2:.:.:mediacapture.pro:use_gstreamer_camera AND QT_FEATURE_linux_v4l:
# OTHER_FILES = "mediacapturecamera.json"

#### Keys ignored in scope 3:.:.:mediacapture.pro:else:
# OTHER_FILES = "mediacapture.json"

qt_internal_extend_target(QGstreamerCaptureServicePlugin CONDITION TARGET Qt::Widgets
    DEFINES
        HAVE_WIDGETS
    PUBLIC_LIBRARIES
        Qt::MultimediaWidgetsPrivate
        Qt::Widgets
)

qt_internal_extend_target(QGstreamerCaptureServicePlugin CONDITION QT_FEATURE_gstreamer_app
    PUBLIC_LIBRARIES
        gstreamer_app
)
