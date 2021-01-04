# Generated from src.pro.

#####################################################################
## QAndroidMediaServicePlugin Plugin:
#####################################################################

qt_internal_add_plugin(QAndroidMediaServicePlugin
    OUTPUT_NAME qtmedia_android
    TYPE mediaservice
    SOURCES
        common/qandroidglobal.h
        common/qandroidmultimediautils.cpp common/qandroidmultimediautils.h
        common/qandroidvideooutput.cpp common/qandroidvideooutput.h
        mediacapture/qandroidaudioencodersettingscontrol.cpp mediacapture/qandroidaudioencodersettingscontrol.h
        mediacapture/qandroidaudioinputselectorcontrol.cpp mediacapture/qandroidaudioinputselectorcontrol.h
        mediacapture/qandroidcameracontrol.cpp mediacapture/qandroidcameracontrol.h
        mediacapture/qandroidcameraexposurecontrol.cpp mediacapture/qandroidcameraexposurecontrol.h
        mediacapture/qandroidcamerafocuscontrol.cpp mediacapture/qandroidcamerafocuscontrol.h
        mediacapture/qandroidcameraimagecapturecontrol.cpp mediacapture/qandroidcameraimagecapturecontrol.h
        mediacapture/qandroidcameraimageprocessingcontrol.cpp mediacapture/qandroidcameraimageprocessingcontrol.h
        mediacapture/qandroidcamerasession.cpp mediacapture/qandroidcamerasession.h
        mediacapture/qandroidcameravideorenderercontrol.cpp mediacapture/qandroidcameravideorenderercontrol.h
        mediacapture/qandroidcaptureservice.cpp mediacapture/qandroidcaptureservice.h
        mediacapture/qandroidcapturesession.cpp mediacapture/qandroidcapturesession.h
        mediacapture/qandroidimageencodercontrol.cpp mediacapture/qandroidimageencodercontrol.h
        mediacapture/qandroidmediacontainercontrol.cpp mediacapture/qandroidmediacontainercontrol.h
        mediacapture/qandroidmediarecordercontrol.cpp mediacapture/qandroidmediarecordercontrol.h
        mediacapture/qandroidmediavideoprobecontrol.cpp mediacapture/qandroidmediavideoprobecontrol.h
        mediacapture/qandroidvideodeviceselectorcontrol.cpp mediacapture/qandroidvideodeviceselectorcontrol.h
        mediacapture/qandroidvideoencodersettingscontrol.cpp mediacapture/qandroidvideoencodersettingscontrol.h
        mediaplayer/qandroidmediaplayercontrol.cpp mediaplayer/qandroidmediaplayercontrol.h
        mediaplayer/qandroidmediaplayervideorenderercontrol.cpp mediaplayer/qandroidmediaplayervideorenderercontrol.h
        mediaplayer/qandroidmediaservice.cpp mediaplayer/qandroidmediaservice.h
        mediaplayer/qandroidmetadatareadercontrol.cpp mediaplayer/qandroidmetadatareadercontrol.h
        qandroidmediaserviceplugin.cpp qandroidmediaserviceplugin.h
        wrappers/jni/androidcamera.cpp wrappers/jni/androidcamera.h
        wrappers/jni/androidmediametadataretriever.cpp wrappers/jni/androidmediametadataretriever.h
        wrappers/jni/androidmediaplayer.cpp wrappers/jni/androidmediaplayer.h
        wrappers/jni/androidmediarecorder.cpp wrappers/jni/androidmediarecorder.h
        wrappers/jni/androidmultimediautils.cpp wrappers/jni/androidmultimediautils.h
        wrappers/jni/androidsurfacetexture.cpp wrappers/jni/androidsurfacetexture.h
        wrappers/jni/androidsurfaceview.cpp wrappers/jni/androidsurfaceview.h
    INCLUDE_DIRECTORIES
        common
        mediacapture
        mediaplayer
        wrappers/jni
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::Network
        Qt::OpenGL
)

#### Keys ignored in scope 1:.:.:src.pro:<TRUE>:
# OTHER_FILES = "android_mediaservice.json"
