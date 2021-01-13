# Generated from multimedia.pro.

#####################################################################
## Multimedia Module:
#####################################################################

qt_internal_add_module(Multimedia
    PLUGIN_TYPES video/gstvideorenderer video/videonode
    SOURCES
        audio/qaudio.cpp audio/qaudio.h
        audio/qaudiobuffer.cpp audio/qaudiobuffer.h audio/qaudiobuffer_p.h
        audio/qaudiodecoder.cpp audio/qaudiodecoder.h
        audio/qaudiodeviceinfo.cpp audio/qaudiodeviceinfo.h audio/qaudiodeviceinfo_p.h
        audio/qaudioformat.cpp audio/qaudioformat.h
        audio/qaudiohelpers.cpp audio/qaudiohelpers_p.h
        audio/qaudioinput.cpp audio/qaudioinput.h
        audio/qaudiooutput.cpp audio/qaudiooutput.h
        audio/qaudioprobe.cpp audio/qaudioprobe.h
        audio/qaudiosystem.cpp audio/qaudiosystem_p.h
        audio/qsamplecache_p.cpp audio/qsamplecache_p.h
        audio/qsoundeffect.cpp audio/qsoundeffect.h
        audio/qwavedecoder_p.cpp audio/qwavedecoder_p.h
        camera/qcamera.cpp camera/qcamera.h camera/qcamera_p.h
        camera/qcameraexposure.cpp camera/qcameraexposure.h
        camera/qcamerafocus.cpp camera/qcamerafocus.h
        camera/qcameraimagecapture.cpp camera/qcameraimagecapture.h
        camera/qcameraimageprocessing.cpp camera/qcameraimageprocessing.h
        camera/qcamerainfo.cpp camera/qcamerainfo.h camera/qcamerainfo_p.h
        camera/qcameraviewfindersettings.cpp camera/qcameraviewfindersettings.h
        controls/qaudiodecodercontrol.cpp controls/qaudiodecodercontrol.h
        controls/qaudioencodersettingscontrol.cpp controls/qaudioencodersettingscontrol.h
        controls/qaudioinputselectorcontrol.cpp controls/qaudioinputselectorcontrol.h
        controls/qaudiooutputselectorcontrol.cpp controls/qaudiooutputselectorcontrol.h
        controls/qcameracontrol.cpp controls/qcameracontrol.h
        controls/qcameraexposurecontrol.cpp controls/qcameraexposurecontrol.h
        controls/qcamerafocuscontrol.cpp controls/qcamerafocuscontrol.h
        controls/qcameraimagecapturecontrol.cpp controls/qcameraimagecapturecontrol.h
        controls/qcameraimageprocessingcontrol.cpp controls/qcameraimageprocessingcontrol.h
        controls/qimageencodercontrol.cpp controls/qimageencodercontrol.h
        controls/qmediaaudioprobecontrol.cpp controls/qmediaaudioprobecontrol.h
        controls/qmediacontainercontrol.cpp controls/qmediacontainercontrol.h
        controls/qmediaplayercontrol.cpp controls/qmediaplayercontrol.h
        controls/qmediarecordercontrol.cpp controls/qmediarecordercontrol.h
        controls/qmediastreamscontrol.cpp controls/qmediastreamscontrol.h
        controls/qmediavideoprobecontrol.cpp controls/qmediavideoprobecontrol.h
        controls/qmetadatareadercontrol.cpp controls/qmetadatareadercontrol.h
        controls/qmetadatawritercontrol.cpp controls/qmetadatawritercontrol.h
        controls/qvideodeviceselectorcontrol.cpp controls/qvideodeviceselectorcontrol.h
        controls/qvideoencodersettingscontrol.cpp controls/qvideoencodersettingscontrol.h
        controls/qvideorenderercontrol.cpp controls/qvideorenderercontrol.h
        controls/qvideowindowcontrol.cpp controls/qvideowindowcontrol.h
        platform/qmediaplatformdevicemanager.cpp platform/qmediaplatformdevicemanager_p.h
        platform/qmediaplatformintegration.cpp platform/qmediaplatformintegration_p.h
        playback/qmediaplayer.cpp playback/qmediaplayer.h
        playback/qmediaplaylist.cpp playback/qmediaplaylist.h playback/qmediaplaylist_p.h
        playback/qplaylistfileparser.cpp playback/qplaylistfileparser_p.h
        qmediadevicemanager.cpp qmediadevicemanager.h
        qmediaenumdebug.h
        qmediametadata.cpp qmediametadata.h
        qmediapluginloader.cpp qmediapluginloader_p.h
        qmediaservice.cpp qmediaservice.h qmediaservice_p.h
        qmediaserviceprovider.cpp qmediaserviceprovider_p.h
        qmediaserviceproviderplugin.h
        qmediasink.cpp qmediasink.h
        qmediasource.cpp qmediasource.h qmediasource_p.h
        qmediastoragelocation.cpp qmediastoragelocation_p.h
        qmediatimerange.cpp qmediatimerange.h
        qmultimedia.cpp qmultimedia.h
        qmultimediautils.cpp qmultimediautils_p.h
        qtmultimediaglobal.h qtmultimediaglobal_p.h
        recording/qmediaencodersettings.cpp recording/qmediaencodersettings.h
        recording/qmediarecorder.cpp recording/qmediarecorder.h recording/qmediarecorder_p.h
        video/qabstractvideobuffer.cpp video/qabstractvideobuffer.h video/qabstractvideobuffer_p.h
        video/qabstractvideofilter.cpp video/qabstractvideofilter.h
        video/qabstractvideosurface.cpp video/qabstractvideosurface.h
        video/qimagevideobuffer.cpp video/qimagevideobuffer_p.h
        video/qmemoryvideobuffer.cpp video/qmemoryvideobuffer_p.h
        video/qvideoframe.cpp video/qvideoframe.h
        video/qvideoframeconversionhelper.cpp video/qvideoframeconversionhelper_p.h
        video/qvideooutputorientationhandler.cpp video/qvideooutputorientationhandler_p.h
        video/qvideoprobe.cpp video/qvideoprobe.h
        video/qvideosurfaceformat.cpp video/qvideosurfaceformat.h
        video/qvideosurfaceoutput.cpp video/qvideosurfaceoutput_p.h
        video/qvideosurfaces.cpp video/qvideosurfaces_p.h
    INCLUDE_DIRECTORIES
        .
        audio
        camera
        controls
        playback
        recording
        video
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Network
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
        Qt::GuiPrivate
)


qt_internal_add_simd_part(Multimedia SIMD sse2
    SOURCES
        video/qvideoframeconversionhelper_sse2.cpp
)

qt_internal_add_simd_part(Multimedia SIMD ssse3
    SOURCES
        video/qvideoframeconversionhelper_ssse3.cpp
)

qt_internal_add_simd_part(Multimedia SIMD avx2
    SOURCES
        video/qvideoframeconversionhelper_avx2.cpp
)


if(ANDROID)
    set_property(TARGET Multimedia APPEND PROPERTY QT_ANDROID_BUNDLED_JAR_DEPENDENCIES
        jar/QtAndroidMultimedia.jar:org.qtproject.qt.android.multimedia.QtMultimediaUtils
    )
    set_property(TARGET Multimedia APPEND PROPERTY QT_ANDROID_LIB_DEPENDENCIES
        lib/libQt5MultimediaQuick.so:Qt5Quick
        plugins/mediaservice/libplugins_mediaservice_qtmedia_android.so
    )
    set_property(TARGET Multimedia APPEND PROPERTY QT_ANDROID_BUNDLED_FILES
        lib/libQt5MultimediaQuick.so
    )
    set_property(TARGET Multimedia APPEND PROPERTY QT_ANDROID_PERMISSIONS
        android.permission.CAMERA android.permission.RECORD_AUDIO
    )
endif()

#### Keys ignored in scope 1:.:.:multimedia.pro:<TRUE>:
# ANDROID_FEATURES = "android.hardware.camera" "android.hardware.camera.autofocus" "android.hardware.microphone"
# MODULE_WINRT_CAPABILITIES_DEVICE = "microphone" "webcam"

## Scopes:
#####################################################################

qt_internal_extend_target(Multimedia CONDITION WIN32
    SOURCES
        platform/wasapi/qwindowsaudiodeviceinfo.cpp platform/wasapi/qwindowsaudiodeviceinfo_p.h
        platform/wasapi/qwindowsaudioinput.cpp platform/wasapi/qwindowsaudioinput_p.h
        platform/wasapi/qwindowsaudiooutput.cpp platform/wasapi/qwindowsaudiooutput_p.h
        platform/wasapi/qwindowsaudioutils.cpp platform/wasapi/qwindowsaudioutils_p.h
        platform/wmf/decoder/mfaudiodecodercontrol.cpp platform/wmf/decoder/mfaudiodecodercontrol_p.h
        platform/wmf/decoder/mfdecodersourcereader.cpp platform/wmf/decoder/mfdecodersourcereader_p.h
        platform/wmf/evr/evrcustompresenter.cpp platform/wmf/evr/evrcustompresenter_p.h
        platform/wmf/evr/evrd3dpresentengine.cpp platform/wmf/evr/evrd3dpresentengine_p.h
        platform/wmf/evr/evrdefs.cpp platform/wmf/evr/evrdefs_p.h
        platform/wmf/evr/evrhelpers.cpp platform/wmf/evr/evrhelpers_p.h
        platform/wmf/evr/evrvideowindowcontrol.cpp platform/wmf/evr/evrvideowindowcontrol_p.h
        platform/wmf/mfstream.cpp platform/wmf/mfstream_p.h
        platform/wmf/player/mfactivate.cpp platform/wmf/player/mfactivate_p.h
        platform/wmf/player/mfaudioendpointcontrol.cpp platform/wmf/player/mfaudioendpointcontrol_p.h
        platform/wmf/player/mfaudioprobecontrol.cpp platform/wmf/player/mfaudioprobecontrol_p.h
        platform/wmf/player/mfevrvideowindowcontrol.cpp platform/wmf/player/mfevrvideowindowcontrol_p.h
        platform/wmf/player/mfmetadatacontrol.cpp platform/wmf/player/mfmetadatacontrol_p.h
        platform/wmf/player/mfplayercontrol.cpp platform/wmf/player/mfplayercontrol_p.h
        platform/wmf/player/mfplayerservice.cpp platform/wmf/player/mfplayerservice_p.h
        platform/wmf/player/mfplayersession.cpp platform/wmf/player/mfplayersession_p.h
        platform/wmf/player/mftvideo.cpp platform/wmf/player/mftvideo_p.h
        platform/wmf/player/mfvideoprobecontrol.cpp platform/wmf/player/mfvideoprobecontrol_p.h
        platform/wmf/player/mfvideorenderercontrol.cpp platform/wmf/player/mfvideorenderercontrol_p.h
        platform/wmf/player/samplegrabber.cpp platform/wmf/player/samplegrabber_p.h
        platform/wmf/qwindowsdevicemanager.cpp platform/wmf/qwindowsdevicemanager_p.h
        platform/wmf/qwindowsintegration.cpp platform/wmf/qwindowsintegration_p.h
        platform/wmf/sourceresolver.cpp platform/wmf/sourceresolver_p.h
        platform/wmf/wmfserviceplugin.cpp platform/wmf/wmfserviceplugin_p.h
    INCLUDE_DIRECTORIES
        .
        platform/wmf/decoder
        platform/wmf/evr
        platform/wmf/player
    LIBRARIES
        Qt::GuiPrivate
        uuid
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Network
        d3d9
        dxva2
        evr
        gdi32
        mf
        mfplat
        mfreadwrite
        mfuuid
        ole32
        oleaut32
        strmiids
        user32
        winmm
        wmcodecdspuuid
        wmf
    PRIVATE_MODULE_INTERFACE
        Qt::GuiPrivate
)

qt_internal_extend_target(Multimedia CONDITION QT_FEATURE_gstreamer
    SOURCES
        platform/gstreamer/audio/qaudiodeviceinfo_gstreamer.cpp platform/gstreamer/audio/qaudiodeviceinfo_gstreamer_p.h
        platform/gstreamer/audio/qaudioengine_gstreamer.cpp platform/gstreamer/audio/qaudioengine_gstreamer_p.h
        platform/gstreamer/audio/qaudioinput_gstreamer.cpp platform/gstreamer/audio/qaudioinput_gstreamer_p.h
        platform/gstreamer/audio/qaudiooutput_gstreamer.cpp platform/gstreamer/audio/qaudiooutput_gstreamer_p.h
        platform/gstreamer/audio/qgstreameraudiodecodercontrol.cpp platform/gstreamer/audio/qgstreameraudiodecodercontrol_p.h
        platform/gstreamer/common/qgstappsrc.cpp platform/gstreamer/common/qgstappsrc_p.h
        platform/gstreamer/common/qgstcodecsinfo.cpp platform/gstreamer/common/qgstcodecsinfo_p.h
        platform/gstreamer/common/qgstreameraudioinputselector.cpp platform/gstreamer/common/qgstreameraudioinputselector_p.h
        platform/gstreamer/common/qgstreameraudioprobecontrol.cpp platform/gstreamer/common/qgstreameraudioprobecontrol_p.h
        platform/gstreamer/common/qgstreamerbufferprobe.cpp platform/gstreamer/common/qgstreamerbufferprobe_p.h
        platform/gstreamer/common/qgstreamerbushelper.cpp platform/gstreamer/common/qgstreamerbushelper_p.h
        platform/gstreamer/common/qgstreamermessage.cpp platform/gstreamer/common/qgstreamermessage_p.h
        platform/gstreamer/common/qgstreamerplayercontrol.cpp platform/gstreamer/common/qgstreamerplayercontrol_p.h
        platform/gstreamer/common/qgstreamerplayersession.cpp platform/gstreamer/common/qgstreamerplayersession_p.h
        platform/gstreamer/common/qgstreamervideoinputdevicecontrol.cpp platform/gstreamer/common/qgstreamervideoinputdevicecontrol_p.h
        platform/gstreamer/common/qgstreamervideooverlay.cpp platform/gstreamer/common/qgstreamervideooverlay_p.h
        platform/gstreamer/common/qgstreamervideoprobecontrol.cpp platform/gstreamer/common/qgstreamervideoprobecontrol_p.h
        platform/gstreamer/common/qgstreamervideorenderer.cpp platform/gstreamer/common/qgstreamervideorenderer_p.h
        platform/gstreamer/common/qgstreamervideorendererinterface.cpp platform/gstreamer/common/qgstreamervideorendererinterface_p.h
        platform/gstreamer/common/qgstreamervideowindow.cpp platform/gstreamer/common/qgstreamervideowindow_p.h
        platform/gstreamer/common/qgstutils.cpp platform/gstreamer/common/qgstutils_p.h
        platform/gstreamer/common/qgstvideobuffer.cpp platform/gstreamer/common/qgstvideobuffer_p.h
        platform/gstreamer/common/qgstvideorendererplugin.cpp platform/gstreamer/common/qgstvideorendererplugin_p.h
        platform/gstreamer/common/qgstvideorenderersink.cpp platform/gstreamer/common/qgstvideorenderersink_p.h
        platform/gstreamer/mediaplayer/qgstreamermetadataprovider.cpp platform/gstreamer/mediaplayer/qgstreamermetadataprovider_p.h
        platform/gstreamer/mediaplayer/qgstreamerplayerservice.cpp platform/gstreamer/mediaplayer/qgstreamerplayerservice_p.h
        platform/gstreamer/mediaplayer/qgstreamerplayerserviceplugin.cpp platform/gstreamer/mediaplayer/qgstreamerplayerserviceplugin_p.h
        platform/gstreamer/mediaplayer/qgstreamerstreamscontrol.cpp platform/gstreamer/mediaplayer/qgstreamerstreamscontrol_p.h
        platform/gstreamer/qgstreamerplatformdevicemanager.cpp platform/gstreamer/qgstreamerplatformdevicemanager_p.h
        platform/gstreamer/qgstreamerplatformintegration.cpp platform/gstreamer/qgstreamerplatformintegration_p.h
    DEFINES
        GLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_26
    INCLUDE_DIRECTORIES
        platform/gstreamer/mediaplayer
    LIBRARIES
        gstreamer
        gstreamer_app
)

qt_internal_extend_target(Multimedia CONDITION QT_FEATURE_gstreamer AND use_camerabin
    SOURCES
        platform/gstreamer/camerabin/camerabinaudioencoder.cpp platform/gstreamer/camerabin/camerabinaudioencoder_p.h
        platform/gstreamer/camerabin/camerabincontainer.cpp platform/gstreamer/camerabin/camerabincontainer_p.h
        platform/gstreamer/camerabin/camerabincontrol.cpp platform/gstreamer/camerabin/camerabincontrol_p.h
        platform/gstreamer/camerabin/camerabinimagecapture.cpp platform/gstreamer/camerabin/camerabinimagecapture_p.h
        platform/gstreamer/camerabin/camerabinimageencoder.cpp platform/gstreamer/camerabin/camerabinimageencoder_p.h
        platform/gstreamer/camerabin/camerabinimageprocessing.cpp platform/gstreamer/camerabin/camerabinimageprocessing_p.h
        platform/gstreamer/camerabin/camerabinmetadata.cpp platform/gstreamer/camerabin/camerabinmetadata_p.h
        platform/gstreamer/camerabin/camerabinrecorder.cpp platform/gstreamer/camerabin/camerabinrecorder_p.h
        platform/gstreamer/camerabin/camerabinservice.cpp platform/gstreamer/camerabin/camerabinservice_p.h
        platform/gstreamer/camerabin/camerabinserviceplugin.cpp platform/gstreamer/camerabin/camerabinserviceplugin_p.h
        platform/gstreamer/camerabin/camerabinsession.cpp platform/gstreamer/camerabin/camerabinsession_p.h
        platform/gstreamer/camerabin/camerabinvideoencoder.cpp platform/gstreamer/camerabin/camerabinvideoencoder_p.h
    DEFINES
        GST_USE_CAMERABIN
)

qt_internal_extend_target(Multimedia CONDITION ((QT_FEATURE_gstreamer) AND (use_camerabin)) AND (qtConfig(gstreamer__p.hotography))
    SOURCES
        platform/gstreamer/camerabin/camerabinexposure.cpp platform/gstreamer/camerabin/camerabinexposure_p.h
        platform/gstreamer/camerabin/camerabinfocus.cpp platform/gstreamer/camerabin/camerabinfocus_p.h
    DEFINES
        GST_USE_UNSTABLE_API
    PUBLIC_LIBRARIES
        gstreamer_photography
)

qt_internal_extend_target(Multimedia CONDITION QT_FEATURE_gstreamer AND QT_FEATURE_gstreamer_gl AND use_camerabin
    PUBLIC_LIBRARIES
        gstreamer_gl
)

qt_internal_extend_target(Multimedia CONDITION QT_FEATURE_gstreamer AND QT_FEATURE_linux_v4l AND use_camerabin
    SOURCES
        platform/gstreamer/camerabin/camerabinv4limageprocessing.cpp platform/gstreamer/camerabin/camerabinv4limageprocessing_p.h
)

qt_internal_extend_target(Multimedia CONDITION QT_FEATURE_gstreamer AND NOT use_camerabin
    SOURCES
        platform/gstreamer/mediacapture/qgstreameraudioencode.cpp platform/gstreamer/mediacapture/qgstreameraudioencode_p.h
        platform/gstreamer/mediacapture/qgstreamercameracontrol.cpp platform/gstreamer/mediacapture/qgstreamercameracontrol_p.h
        platform/gstreamer/mediacapture/qgstreamercapturemetadatacontrol.cpp platform/gstreamer/mediacapture/qgstreamercapturemetadatacontrol_p.h
        platform/gstreamer/mediacapture/qgstreamercaptureservice.cpp platform/gstreamer/mediacapture/qgstreamercaptureservice_p.h
        platform/gstreamer/mediacapture/qgstreamercaptureserviceplugin.cpp platform/gstreamer/mediacapture/qgstreamercaptureserviceplugin_p.h
        platform/gstreamer/mediacapture/qgstreamercapturesession.cpp platform/gstreamer/mediacapture/qgstreamercapturesession_p.h
        platform/gstreamer/mediacapture/qgstreamerimagecapturecontrol.cpp platform/gstreamer/mediacapture/qgstreamerimagecapturecontrol_p.h
        platform/gstreamer/mediacapture/qgstreamerimageencode.cpp platform/gstreamer/mediacapture/qgstreamerimageencode_p.h
        platform/gstreamer/mediacapture/qgstreamermediacontainercontrol.cpp platform/gstreamer/mediacapture/qgstreamermediacontainercontrol_p.h
        platform/gstreamer/mediacapture/qgstreamerrecordercontrol.cpp platform/gstreamer/mediacapture/qgstreamerrecordercontrol_p.h
        platform/gstreamer/mediacapture/qgstreamervideoencode.cpp platform/gstreamer/mediacapture/qgstreamervideoencode_p.h
    INCLUDE_DIRECTORIES
        platform/gstreamer/mediacapture
)

qt_internal_extend_target(Multimedia CONDITION QT_FEATURE_gstreamer AND QT_FEATURE_linux_v4l AND use_gstreamer_camera AND NOT use_camerabin
    SOURCES
        platform/gstreamer/mediacapture/qgstreamerv4l2input.cpp platform/gstreamer/mediacapture/qgstreamerv4l2input_p.h
    DEFINES
        USE_GSTREAMER_CAMERA
)

qt_internal_extend_target(Multimedia CONDITION QT_FEATURE_gstreamer AND QT_FEATURE_gstreamer_gl
    PUBLIC_LIBRARIES
        gstreamer_gl
)

qt_internal_extend_target(Multimedia CONDITION ANDROID AND QT_FEATURE_gstreamer
    LIBRARIES
        # Remove: L$ENV{GSTREAMER_ROOT_ANDROID}/armv7/lib
        # Remove: Wl,--_p.hole-archive
        # Remove: Wl,--no-_p.hole-archive
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

qt_internal_extend_target(Multimedia CONDITION QT_FEATURE_pulseaudio
    SOURCES
        platform/pulseaudio/qaudiodeviceinfo_pulse.cpp platform/pulseaudio/qaudiodeviceinfo_pulse_p.h
        platform/pulseaudio/qaudioengine_pulse.cpp platform/pulseaudio/qaudioengine_pulse_p.h
        platform/pulseaudio/qaudioinput_pulse.cpp platform/pulseaudio/qaudioinput_pulse_p.h
        platform/pulseaudio/qaudiooutput_pulse.cpp platform/pulseaudio/qaudiooutput_pulse_p.h
        platform/pulseaudio/qpulseaudiodevicemanager.cpp platform/pulseaudio/qpulseaudiodevicemanager_p.h
        platform/pulseaudio/qpulseaudiointegration.cpp platform/pulseaudio/qpulseaudiointegration_p.h
        platform/pulseaudio/qpulsehelpers.cpp platform/pulseaudio/qpulsehelpers_p.h
    LIBRARIES
        pulseaudio
)

qt_internal_extend_target(Multimedia CONDITION QT_FEATURE_alsa
    SOURCES
        platform/alsa/qalsaaudiodeviceinfo.cpp platform/alsa/qalsaaudiodeviceinfo_p.h
        platform/alsa/qalsaaudioinput.cpp platform/alsa/qalsaaudioinput_p.h
        platform/alsa/qalsaaudiooutput.cpp platform/alsa/qalsaaudiooutput_p.h
        platform/alsa/qalsadevicemanager.cpp platform/alsa/qalsadevicemanager_p.h
        platform/alsa/qalsaintegration.cpp platform/alsa/qalsaintegration_p.h
    LIBRARIES
        alsa
)

qt_internal_extend_target(Multimedia CONDITION ANDROID
    SOURCES
        platform/android/common/qandroidglobal_p.h
        platform/android/common/qandroidmultimediautils.cpp platform/android/common/qandroidmultimediautils_p.h
        platform/android/common/qandroidvideooutput.cpp platform/android/common/qandroidvideooutput_p.h
        platform/android/mediacapture/qandroidaudioencodersettingscontrol.cpp platform/android/mediacapture/qandroidaudioencodersettingscontrol_p.h
        platform/android/mediacapture/qandroidaudioinputselectorcontrol.cpp platform/android/mediacapture/qandroidaudioinputselectorcontrol_p.h
        platform/android/mediacapture/qandroidcameracontrol.cpp platform/android/mediacapture/qandroidcameracontrol_p.h
        platform/android/mediacapture/qandroidcameraexposurecontrol.cpp platform/android/mediacapture/qandroidcameraexposurecontrol_p.h
        platform/android/mediacapture/qandroidcamerafocuscontrol.cpp platform/android/mediacapture/qandroidcamerafocuscontrol_p.h
        platform/android/mediacapture/qandroidcameraimagecapturecontrol.cpp platform/android/mediacapture/qandroidcameraimagecapturecontrol_p.h
        platform/android/mediacapture/qandroidcameraimageprocessingcontrol.cpp platform/android/mediacapture/qandroidcameraimageprocessingcontrol_p.h
        platform/android/mediacapture/qandroidcamerasession.cpp platform/android/mediacapture/qandroidcamerasession_p.h
        platform/android/mediacapture/qandroidcameravideorenderercontrol.cpp platform/android/mediacapture/qandroidcameravideorenderercontrol_p.h
        platform/android/mediacapture/qandroidcaptureservice.cpp platform/android/mediacapture/qandroidcaptureservice_p.h
        platform/android/mediacapture/qandroidcapturesession.cpp platform/android/mediacapture/qandroidcapturesession_p.h
        platform/android/mediacapture/qandroidimageencodercontrol.cpp platform/android/mediacapture/qandroidimageencodercontrol_p.h
        platform/android/mediacapture/qandroidmediacontainercontrol.cpp platform/android/mediacapture/qandroidmediacontainercontrol_p.h
        platform/android/mediacapture/qandroidmediarecordercontrol.cpp platform/android/mediacapture/qandroidmediarecordercontrol_p.h
        platform/android/mediacapture/qandroidmediavideoprobecontrol.cpp platform/android/mediacapture/qandroidmediavideoprobecontrol_p.h
        platform/android/mediacapture/qandroidvideodeviceselectorcontrol.cpp platform/android/mediacapture/qandroidvideodeviceselectorcontrol_p.h
        platform/android/mediacapture/qandroidvideoencodersettingscontrol.cpp platform/android/mediacapture/qandroidvideoencodersettingscontrol_p.h
        platform/android/mediaplayer/qandroidmediaplayercontrol.cpp platform/android/mediaplayer/qandroidmediaplayercontrol_p.h
        platform/android/mediaplayer/qandroidmediaplayervideorenderercontrol.cpp platform/android/mediaplayer/qandroidmediaplayervideorenderercontrol_p.h
        platform/android/mediaplayer/qandroidmediaservice.cpp platform/android/mediaplayer/qandroidmediaservice_p.h
        platform/android/mediaplayer/qandroidmetadatareadercontrol.cpp platform/android/mediaplayer/qandroidmetadatareadercontrol_p.h
        platform/android/qandroidmediaserviceplugin.cpp platform/android/qandroidmediaserviceplugin_p.h
        platform/android/wrappers/jni/androidcamera.cpp platform/android/wrappers/jni/androidcamera_p.h
        platform/android/wrappers/jni/androidmediametadataretriever.cpp platform/android/wrappers/jni/androidmediametadataretriever_p.h
        platform/android/wrappers/jni/androidmediaplayer.cpp platform/android/wrappers/jni/androidmediaplayer_p.h
        platform/android/wrappers/jni/androidmediarecorder.cpp platform/android/wrappers/jni/androidmediarecorder_p.h
        platform/android/wrappers/jni/androidmultimediautils.cpp platform/android/wrappers/jni/androidmultimediautils_p.h
        platform/android/wrappers/jni/androidsurfacetexture.cpp platform/android/wrappers/jni/androidsurfacetexture_p.h
        platform/android/wrappers/jni/androidsurfaceview.cpp platform/android/wrappers/jni/androidsurfaceview_p.h
        platform/opensles/qopenslesaudioinput.cpp platform/opensles/qopenslesaudioinput_p.h
        platform/opensles/qopenslesaudiooutput.cpp platform/opensles/qopenslesaudiooutput_p.h
        platform/opensles/qopenslesdeviceinfo.cpp platform/opensles/qopenslesdeviceinfo_p.h
        platform/opensles/qopenslesengine.cpp platform/opensles/qopenslesengine_p.h
    INCLUDE_DIRECTORIES
        platform/android/common
        platform/android/mediacapture
        platform/android/mediaplayer
        platform/android/wrappers/jni
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        OpenSLES
        Qt::Core
        Qt::Network
        Qt::OpenGL
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
)
qt_internal_find_apple_system_framework(FWCoreAudio CoreAudio) # special case

qt_internal_extend_target(Multimedia CONDITION WIN32 AND NOT TARGET Qt::OpenGL
    LIBRARIES
        gdi32
        user32
)

qt_internal_extend_target(Multimedia CONDITION TARGET Qt::Widgets AND WIN32
    PUBLIC_LIBRARIES
        Qt::Widgets
)

qt_internal_extend_target(Multimedia CONDITION APPLE AND NOT WATCHOS
    SOURCES
        platform/avfoundation/mediaplayer/avfmediaplayercontrol.mm platform/avfoundation/mediaplayer/avfmediaplayercontrol_p.h
        platform/avfoundation/mediaplayer/avfmediaplayermetadatacontrol.mm platform/avfoundation/mediaplayer/avfmediaplayermetadatacontrol_p.h
        platform/avfoundation/mediaplayer/avfmediaplayerservice.mm platform/avfoundation/mediaplayer/avfmediaplayerservice_p.h
        platform/avfoundation/mediaplayer/avfmediaplayerserviceplugin.mm platform/avfoundation/mediaplayer/avfmediaplayerserviceplugin_p.h
        platform/avfoundation/mediaplayer/avfmediaplayersession.mm platform/avfoundation/mediaplayer/avfmediaplayersession_p.h
        platform/avfoundation/mediaplayer/avfvideooutput.mm platform/avfoundation/mediaplayer/avfvideooutput_p.h
        platform/avfoundation/mediaplayer/avfvideowindowcontrol.mm platform/avfoundation/mediaplayer/avfvideowindowcontrol_p.h
        platform/avfoundation/qavfplatformdevicemanager.mm platform/avfoundation/qavfplatformdevicemanager_p.h
        platform/avfoundation/qavfplatformintegration.cpp platform/avfoundation/qavfplatformintegration_p.h
        platform/coreaudio/qcoreaudiodeviceinfo.mm platform/coreaudio/qcoreaudiodeviceinfo_p.h
        platform/coreaudio/qcoreaudioinput.mm platform/coreaudio/qcoreaudioinput_p.h
        platform/coreaudio/qcoreaudiooutput.mm platform/coreaudio/qcoreaudiooutput_p.h
        platform/coreaudio/qcoreaudioutils.mm platform/coreaudio/qcoreaudioutils_p.h
    PUBLIC_LIBRARIES
        ${FWAudioToolbox}
        ${FWCoreAudio}
        ${FWCoreFoundation}
        ${FWCoreMedia}
        ${FWCoreVideo}
        ${FWFoundation}
        ${FWMetal}
        ${FWQuartzCore}
        Qt::Network
        Qt::OpenGL
        avfoundation
)

qt_internal_extend_target(Multimedia CONDITION APPLE AND NOT WATCHOS AND (IOS OR TVOS)
    SOURCES
        platform/coreaudio/qcoreaudiosessionmanager.mm platform/coreaudio/qcoreaudiosessionmanager_p.h
    PUBLIC_LIBRARIES
        ${FWAVFoundation}
        ${FWFoundation}
)

qt_internal_extend_target(Multimedia CONDITION APPLE AND NOT IOS AND NOT TVOS AND NOT WATCHOS
    PUBLIC_LIBRARIES
        ${FWAppKit}
        ${FWApplicationServices}
        ${FWAudioUnit}
)

qt_internal_extend_target(Multimedia CONDITION MACOS AND NOT WATCHOS
    PUBLIC_LIBRARIES
        ${FWAppKit}
        ${FWAudioUnit}
)

qt_internal_extend_target(Multimedia CONDITION IOS AND NOT WATCHOS
    PUBLIC_LIBRARIES
        ${FWCoreGraphics}
        ${FWCoreVideo}
)

qt_internal_extend_target(Multimedia CONDITION APPLE AND NOT TVOS AND NOT WATCHOS
    SOURCES
        platform/avfoundation/camera/avfaudioencodersettingscontrol.mm platform/avfoundation/camera/avfaudioencodersettingscontrol_p.h
        platform/avfoundation/camera/avfaudioinputselectorcontrol.mm platform/avfoundation/camera/avfaudioinputselectorcontrol_p.h
        platform/avfoundation/camera/avfcameracontrol.mm platform/avfoundation/camera/avfcameracontrol_p.h
        platform/avfoundation/camera/avfcameradebug_p.h
        platform/avfoundation/camera/avfcameradevicecontrol.mm platform/avfoundation/camera/avfcameradevicecontrol_p.h
        platform/avfoundation/camera/avfcameraexposurecontrol.mm platform/avfoundation/camera/avfcameraexposurecontrol_p.h
        platform/avfoundation/camera/avfcamerafocuscontrol.mm platform/avfoundation/camera/avfcamerafocuscontrol_p.h
        platform/avfoundation/camera/avfcamerametadatacontrol.mm platform/avfoundation/camera/avfcamerametadatacontrol_p.h
        platform/avfoundation/camera/avfcamerarenderercontrol.mm platform/avfoundation/camera/avfcamerarenderercontrol_p.h
        platform/avfoundation/camera/avfcameraservice.mm platform/avfoundation/camera/avfcameraservice_p.h
        platform/avfoundation/camera/avfcameraserviceplugin.mm platform/avfoundation/camera/avfcameraserviceplugin_p.h
        platform/avfoundation/camera/avfcamerasession.mm platform/avfoundation/camera/avfcamerasession_p.h
        platform/avfoundation/camera/avfcamerautility.mm platform/avfoundation/camera/avfcamerautility_p.h
        platform/avfoundation/camera/avfcamerawindowcontrol.mm platform/avfoundation/camera/avfcamerawindowcontrol_p.h
        platform/avfoundation/camera/avfimagecapturecontrol.mm platform/avfoundation/camera/avfimagecapturecontrol_p.h
        platform/avfoundation/camera/avfimageencodercontrol.mm platform/avfoundation/camera/avfimageencodercontrol_p.h
        platform/avfoundation/camera/avfmediacontainercontrol.mm platform/avfoundation/camera/avfmediacontainercontrol_p.h
        platform/avfoundation/camera/avfmediavideoprobecontrol.mm platform/avfoundation/camera/avfmediavideoprobecontrol_p.h
        platform/avfoundation/camera/avfstoragelocation.mm platform/avfoundation/camera/avfstoragelocation_p.h
        platform/avfoundation/camera/avfvideoencodersettingscontrol.mm platform/avfoundation/camera/avfvideoencodersettingscontrol_p.h
)

qt_internal_extend_target(Multimedia CONDITION MACOS AND NOT TVOS AND NOT WATCHOS
    SOURCES
        platform/avfoundation/camera/avfmediarecordercontrol.mm platform/avfoundation/camera/avfmediarecordercontrol_p.h
)

qt_internal_extend_target(Multimedia CONDITION IOS AND NOT TVOS AND NOT WATCHOS
    SOURCES
        platform/avfoundation/camera/avfmediaassetwriter.mm platform/avfoundation/camera/avfmediaassetwriter_p.h
        platform/avfoundation/camera/avfmediarecordercontrol_ios.mm platform/avfoundation/camera/avfmediarecordercontrol_ios_p.h
)

qt_internal_extend_target(Multimedia CONDITION APPLE AND QT_FEATURE_opengl AND NOT WATCHOS AND (IOS OR TVOS)
    SOURCES
        platform/avfoundation/mediaplayer/avfdisplaylink.mm platform/avfoundation/mediaplayer/avfdisplaylink_p.h
        platform/avfoundation/mediaplayer/avfvideoframerenderer_ios.mm platform/avfoundation/mediaplayer/avfvideoframerenderer_ios_p.h
        platform/avfoundation/mediaplayer/avfvideorenderercontrol.mm platform/avfoundation/mediaplayer/avfvideorenderercontrol_p.h
)

qt_internal_extend_target(Multimedia CONDITION APPLE AND QT_FEATURE_opengl AND NOT IOS AND NOT TVOS AND NOT WATCHOS
    SOURCES
        platform/avfoundation/mediaplayer/avfdisplaylink.mm platform/avfoundation/mediaplayer/avfdisplaylink_p.h
        platform/avfoundation/mediaplayer/avfvideoframerenderer.mm platform/avfoundation/mediaplayer/avfvideoframerenderer_p.h
        platform/avfoundation/mediaplayer/avfvideorenderercontrol.mm platform/avfoundation/mediaplayer/avfvideorenderercontrol_p.h
)

qt_internal_extend_target(Multimedia CONDITION QNX
    SOURCES
        platform/qnx/camera/bbcameraaudioencodersettingscontrol.cpp platform/qnx/camera/bbcameraaudioencodersettingscontrol_p.h
        platform/qnx/camera/bbcameracontrol.cpp platform/qnx/camera/bbcameracontrol_p.h
        platform/qnx/camera/bbcameraexposurecontrol.cpp platform/qnx/camera/bbcameraexposurecontrol_p.h
        platform/qnx/camera/bbcamerafocuscontrol.cpp platform/qnx/camera/bbcamerafocuscontrol_p.h
        platform/qnx/camera/bbcameraimagecapturecontrol.cpp platform/qnx/camera/bbcameraimagecapturecontrol_p.h
        platform/qnx/camera/bbcameraimageprocessingcontrol.cpp platform/qnx/camera/bbcameraimageprocessingcontrol_p.h
        platform/qnx/camera/bbcameramediarecordercontrol.cpp platform/qnx/camera/bbcameramediarecordercontrol_p.h
        platform/qnx/camera/bbcameraorientatio_p.handler.cpp platform/qnx/camera/bbcameraorientatio_p.handler.h
        platform/qnx/camera/bbcameraservice.cpp platform/qnx/camera/bbcameraservice_p.h
        platform/qnx/camera/bbcamerasession.cpp platform/qnx/camera/bbcamerasession_p.h
        platform/qnx/camera/bbcameravideoencodersettingscontrol.cpp platform/qnx/camera/bbcameravideoencodersettingscontrol_p.h
        platform/qnx/camera/bbcameraviewfindersettingscontrol.cpp platform/qnx/camera/bbcameraviewfindersettingscontrol_p.h
        platform/qnx/camera/bbimageencodercontrol.cpp platform/qnx/camera/bbimageencodercontrol_p.h
        platform/qnx/camera/bbmediastoragelocation.cpp platform/qnx/camera/bbmediastoragelocation_p.h
        platform/qnx/camera/bbvideodeviceselectorcontrol.cpp platform/qnx/camera/bbvideodeviceselectorcontrol_p.h
        platform/qnx/camera/bbvideorenderercontrol.cpp platform/qnx/camera/bbvideorenderercontrol_p.h
        platform/qnx/common/windowgrabber.cpp platform/qnx/common/windowgrabber_p.h
        platform/qnx/mediaplayer/mmrenderermediaplayercontrol.cpp platform/qnx/mediaplayer/mmrenderermediaplayercontrol_p.h
        platform/qnx/mediaplayer/mmrenderermediaplayerservice.cpp platform/qnx/mediaplayer/mmrenderermediaplayerservice_p.h
        platform/qnx/mediaplayer/mmrenderermetadata.cpp platform/qnx/mediaplayer/mmrenderermetadata_p.h
        platform/qnx/mediaplayer/mmrenderermetadatareadercontrol.cpp platform/qnx/mediaplayer/mmrenderermetadatareadercontrol_p.h
        platform/qnx/mediaplayer/mmrendererplayervideorenderercontrol.cpp platform/qnx/mediaplayer/mmrendererplayervideorenderercontrol_p.h
        platform/qnx/mediaplayer/mmrendererutil.cpp platform/qnx/mediaplayer/mmrendererutil_p.h
        platform/qnx/mediaplayer/mmrenderervideowindowcontrol.cpp platform/qnx/mediaplayer/mmrenderervideowindowcontrol_p.h
        platform/qnx/mediaplayer/mmrevent_p.hread.cpp platform/qnx/mediaplayer/mmrevent_p.hread.h
        platform/qnx/mediaplayer/mmreventmediaplayercontrol.cpp platform/qnx/mediaplayer/mmreventmediaplayercontrol_p.h
        platform/qnx/neutrinoserviceplugin.cpp platform/qnx/neutrinoserviceplugin_p.h
        platform/qnx/qnxaudiodeviceinfo.cpp platform/qnx/qnxaudiodeviceinfo_p.h
        platform/qnx/qnxaudioinput.cpp platform/qnx/qnxaudioinput_p.h
        platform/qnx/qnxaudiooutput.cpp platform/qnx/qnxaudiooutput_p.h
        platform/qnx/qnxaudioutils.cpp platform/qnx/qnxaudioutils_p.h
    INCLUDE_DIRECTORIES
        platform/qnx/camera
        platform/qnx/common
        platform/qnx/mediaplayer
    PUBLIC_LIBRARIES
        asound
        audio_manager
        camapi
        mmrenderer
)
qt_internal_add_docs(Multimedia
    doc/qtmultimedia.qdocconf
)

