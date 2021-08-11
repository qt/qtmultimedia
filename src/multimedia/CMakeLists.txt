# Generated from multimedia.pro.

#####################################################################
## Multimedia Module:
#####################################################################

find_package(Qt6 COMPONENTS ShaderTools)

qt_internal_find_apple_system_framework(FWCoreMedia CoreMedia) # special case
qt_internal_find_apple_system_framework(FWCoreAudio CoreAudio) # special case
qt_internal_find_apple_system_framework(FWAudioUnit AudioUnit) # special case
qt_internal_find_apple_system_framework(FWVideoToolbox VideoToolbox) # special case
qt_internal_find_apple_system_framework(FWAVFoundation AVFoundation) # special case

qt_internal_add_module(Multimedia
    PLUGIN_TYPES video/gstvideorenderer video/videonode
    SOURCES
        audio/qaudio.cpp audio/qaudio.h
        audio/qaudiobuffer.cpp audio/qaudiobuffer.h
        audio/qaudiodecoder.cpp audio/qaudiodecoder.h
        audio/qaudiodevice.cpp audio/qaudiodevice.h audio/qaudiodevice_p.h
        audio/qaudioinput.cpp audio/qaudioinput.h
        audio/qaudiooutput.cpp audio/qaudiooutput.h
        audio/qaudioformat.cpp audio/qaudioformat.h
        audio/qaudiohelpers.cpp audio/qaudiohelpers_p.h
        audio/qaudiosource.cpp audio/qaudiosource.h
        audio/qaudiosink.cpp audio/qaudiosink.h
        audio/qaudiosystem.cpp audio/qaudiosystem_p.h
        audio/qsamplecache_p.cpp audio/qsamplecache_p.h
        audio/qsoundeffect.cpp audio/qsoundeffect.h
        audio/qwavedecoder.cpp audio/qwavedecoder.h
        camera/qcamera.cpp camera/qcamera.h camera/qcamera_p.h
        camera/qcameradevice.cpp camera/qcameradevice.h camera/qcameradevice_p.h
        camera/qimagecapture.cpp camera/qimagecapture.h
        platform/qplatformaudiodecoder.cpp platform/qplatformaudiodecoder_p.h
        platform/qplatformaudioinput_p.h
        platform/qplatformaudiooutput_p.h
        platform/qplatformcamera.cpp platform/qplatformcamera_p.h
        platform/qplatformimagecapture.cpp platform/qplatformimagecapture_p.h
        platform/qplatformmediacapture.cpp platform/qplatformmediacapture_p.h
        platform/qplatformmediadevices.cpp platform/qplatformmediadevices_p.h
        platform/qplatformmediarecorder.cpp platform/qplatformmediarecorder_p.h
        platform/qplatformmediaformatinfo.cpp  platform/qplatformmediaformatinfo_p.h
        platform/qplatformmediaintegration.cpp platform/qplatformmediaintegration_p.h
        platform/qplatformmediaplayer.cpp platform/qplatformmediaplayer_p.h
        platform/qplatformvideosink.cpp platform/qplatformvideosink_p.h
        playback/qmediaplayer.cpp playback/qmediaplayer.h playback/qmediaplayer_p.h
        qmediadevices.cpp qmediadevices.h
        qmediaenumdebug.h
        qiso639_2.cpp qiso639_2_p.h
        qmediaformat.cpp  qmediaformat.h
        qmediametadata.cpp qmediametadata.h
        qmediastoragelocation.cpp qmediastoragelocation_p.h
        qmediatimerange.cpp qmediatimerange.h
        qmultimediautils.cpp qmultimediautils_p.h
        qtmultimediaglobal.h qtmultimediaglobal_p.h
        recording/qmediacapturesession.cpp recording/qmediacapturesession.h
        recording/qmediarecorder.cpp recording/qmediarecorder.h recording/qmediarecorder_p.h
        video/qabstractvideobuffer.cpp video/qabstractvideobuffer_p.h
        video/qmemoryvideobuffer.cpp video/qmemoryvideobuffer_p.h
        video/qvideoframe.cpp video/qvideoframe.h
        video/qvideosink.cpp video/qvideosink.h
        video/qvideotexturehelper.cpp video/qvideotexturehelper_p.h
        video/qvideoframeconversionhelper.cpp video/qvideoframeconversionhelper_p.h
        video/qvideooutputorientationhandler.cpp video/qvideooutputorientationhandler_p.h
        video/qvideoframeformat.cpp video/qvideoframeformat.h
        video/qvideowindow.cpp video/qvideowindow_p.h
    INCLUDE_DIRECTORIES
        audio
        camera
        controls
        platform
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
        jar/Qt${QtMultimedia_VERSION_MAJOR}AndroidMultimedia.jar:org.qtproject.qt.android.multimedia.QtMultimediaUtils
        jar/Qt${QtMultimedia_VERSION_MAJOR}AndroidMultimedia.jar:org.qtproject.qt.android.multimedia.QtAudioDeviceManager
    )
    set_property(TARGET Multimedia APPEND PROPERTY QT_ANDROID_LIB_DEPENDENCIES
        lib/libQt6MultimediaQuick.so:Qt6Quick
    )
    set_property(TARGET Multimedia APPEND PROPERTY QT_ANDROID_BUNDLED_FILES
        lib/libQt6MultimediaQuick.so
    )
    set_property(TARGET Multimedia APPEND PROPERTY QT_ANDROID_PERMISSIONS
        android.permission.CAMERA android.permission.RECORD_AUDIO
        android.permission.BLUETOOTH
        android.permission.MODIFY_AUDIO_SETTINGS
    )
endif()

#### Keys ignored in scope 1:.:.:multimedia.pro:<TRUE>:
# ANDROID_FEATURES = "android.hardware.camera" "android.hardware.camera.autofocus" "android.hardware.microphone"
# MODULE_WINRT_CAPABILITIES_DEVICE = "microphone" "webcam"

## Scopes:
#####################################################################

qt_internal_extend_target(Multimedia CONDITION QT_FEATURE_wmf
    SOURCES
        platform/windows/audio/qwindowsaudiodevice.cpp platform/windows/audio/qwindowsaudiodevice_p.h
        platform/windows/audio/qwindowsaudiosource.cpp platform/windows/audio/qwindowsaudiosource_p.h
        platform/windows/audio/qwindowsaudiosink.cpp platform/windows/audio/qwindowsaudiosink_p.h
        platform/windows/audio/qwindowsaudioutils.cpp platform/windows/audio/qwindowsaudioutils_p.h
        platform/windows/common/mfmetadata.cpp platform/windows/common/mfmetadata_p.h
        platform/windows/common/qwindowsmultimediautils.cpp platform/windows/common/qwindowsmultimediautils_p.h
        platform/windows/common/qwindowsmfdefs.cpp platform/windows/common/qwindowsmfdefs_p.h
        platform/windows/common/qwindowsiupointer_p.h
        platform/windows/common/qwindowsresampler.cpp platform/windows/common/qwindowsresampler_p.h
        platform/windows/decoder/mfaudiodecodercontrol.cpp platform/windows/decoder/mfaudiodecodercontrol_p.h
        platform/windows/decoder/mfdecodersourcereader.cpp platform/windows/decoder/mfdecodersourcereader_p.h
        platform/windows/evr/evrcustompresenter.cpp platform/windows/evr/evrcustompresenter_p.h
        platform/windows/evr/evrd3dpresentengine.cpp platform/windows/evr/evrd3dpresentengine_p.h
        platform/windows/evr/evrhelpers.cpp platform/windows/evr/evrhelpers_p.h
        platform/windows/evr/evrvideowindowcontrol.cpp platform/windows/evr/evrvideowindowcontrol_p.h
        platform/windows/mfstream.cpp platform/windows/mfstream_p.h
        platform/windows/player/mfactivate.cpp platform/windows/player/mfactivate_p.h
        platform/windows/player/mfevrvideowindowcontrol.cpp platform/windows/player/mfevrvideowindowcontrol_p.h
        platform/windows/player/mfplayercontrol.cpp platform/windows/player/mfplayercontrol_p.h
        platform/windows/player/mfplayersession.cpp platform/windows/player/mfplayersession_p.h
        platform/windows/player/mftvideo.cpp platform/windows/player/mftvideo_p.h
        platform/windows/player/mfvideorenderercontrol.cpp platform/windows/player/mfvideorenderercontrol_p.h
        platform/windows/player/samplegrabber.cpp platform/windows/player/samplegrabber_p.h
        platform/windows/mediacapture/qwindowscamera.cpp
        platform/windows/mediacapture/qwindowscamera_p.h
        platform/windows/mediacapture/qwindowsimagecapture.cpp
        platform/windows/mediacapture/qwindowsimagecapture_p.h
        platform/windows/mediacapture/qwindowsmediacapture.cpp
        platform/windows/mediacapture/qwindowsmediacapture_p.h
        platform/windows/mediacapture/qwindowsmediadevicereader.cpp
        platform/windows/mediacapture/qwindowsmediadevicereader_p.h
        platform/windows/mediacapture/qwindowsmediadevicesession.cpp
        platform/windows/mediacapture/qwindowsmediadevicesession_p.h
        platform/windows/mediacapture/qwindowsmediaencoder.cpp
        platform/windows/mediacapture/qwindowsmediaencoder_p.h
        platform/windows/qwindowsmediadevices.cpp platform/windows/qwindowsmediadevices_p.h
        platform/windows/qwindowsformatinfo.cpp platform/windows/qwindowsformatinfo_p.h
        platform/windows/qwindowsintegration.cpp platform/windows/qwindowsintegration_p.h
        platform/windows/sourceresolver.cpp platform/windows/sourceresolver_p.h
    INCLUDE_DIRECTORIES
        platform/windows/common
        platform/windows/decoder
        platform/windows/evr
        platform/windows/player
        platform/windows/mediacapture
    LIBRARIES
        uuid
        WMF::WMF
        d3d9
        dxva2
        evr
        gdi32
        ksuser
        mf
        mfcore
        mfplat
        mfreadwrite
        mfuuid
        ole32
        oleaut32
        propsys
        shlwapi
        strmiids
        amstrmid
        user32
        winmm
        wmcodecdspuuid
)

qt_internal_extend_target(Multimedia CONDITION QT_FEATURE_gstreamer
    SOURCES
        platform/gstreamer/audio/qgstreameraudiodevice.cpp platform/gstreamer/audio/qgstreameraudiodevice_p.h
        platform/gstreamer/audio/qgstreameraudiosource.cpp platform/gstreamer/audio/qgstreameraudiosource_p.h
        platform/gstreamer/audio/qgstreameraudiosink.cpp platform/gstreamer/audio/qgstreameraudiosink_p.h
        platform/gstreamer/audio/qgstreameraudiodecoder.cpp platform/gstreamer/audio/qgstreameraudiodecoder_p.h
        platform/gstreamer/common/qgst_p.h
        platform/gstreamer/common/qgstappsrc.cpp platform/gstreamer/common/qgstappsrc_p.h
        platform/gstreamer/common/qgstreameraudioinput.cpp platform/gstreamer/common/qgstreameraudioinput_p.h
        platform/gstreamer/common/qgstreameraudiooutput.cpp platform/gstreamer/common/qgstreameraudiooutput_p.h
        platform/gstreamer/common/qgstreamerbufferprobe.cpp platform/gstreamer/common/qgstreamerbufferprobe_p.h
        platform/gstreamer/common/qgstreamermetadata.cpp platform/gstreamer/common/qgstreamermetadata_p.h
        platform/gstreamer/common/qgstreamermessage.cpp platform/gstreamer/common/qgstreamermessage_p.h
        platform/gstreamer/common/qgstreamermediaplayer.cpp platform/gstreamer/common/qgstreamermediaplayer_p.h
        platform/gstreamer/common/qgstreamervideooutput.cpp platform/gstreamer/common/qgstreamervideooutput_p.h
        platform/gstreamer/common/qgstreamervideooverlay.cpp platform/gstreamer/common/qgstreamervideooverlay_p.h
        platform/gstreamer/common/qgstreamervideosink.cpp platform/gstreamer/common/qgstreamervideosink_p.h
        platform/gstreamer/common/qgstpipeline.cpp platform/gstreamer/common/qgstpipeline_p.h
        platform/gstreamer/common/qgstutils.cpp platform/gstreamer/common/qgstutils_p.h
        platform/gstreamer/common/qgstvideobuffer.cpp platform/gstreamer/common/qgstvideobuffer_p.h
        platform/gstreamer/common/qgstvideorenderersink.cpp platform/gstreamer/common/qgstvideorenderersink_p.h
        platform/gstreamer/common/qgstsubtitlesink.cpp platform/gstreamer/common/qgstsubtitlesink_p.h
        platform/gstreamer/qgstreamermediadevices.cpp platform/gstreamer/qgstreamermediadevices_p.h
        platform/gstreamer/qgstreamerformatinfo.cpp platform/gstreamer/qgstreamerformatinfo_p.h
        platform/gstreamer/qgstreamerintegration.cpp platform/gstreamer/qgstreamerintegration_p.h
        platform/gstreamer/mediacapture/qgstreamercamera.cpp platform/gstreamer/mediacapture/qgstreamercamera_p.h
        platform/gstreamer/mediacapture/qgstreamerimagecapture.cpp platform/gstreamer/mediacapture/qgstreamerimagecapture_p.h
        platform/gstreamer/mediacapture/qgstreamermediacapture.cpp platform/gstreamer/mediacapture/qgstreamermediacapture_p.h
        platform/gstreamer/mediacapture/qgstreamermediaencoder.cpp platform/gstreamer/mediacapture/qgstreamermediaencoder_p.h
    DEFINES
        GLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_26
    INCLUDE_DIRECTORIES
        platform/gstreamer/mediaplayer
        platform/gstreamer/mediacapture
    LIBRARIES
        GStreamer::GStreamer # special case
        GStreamer::App # special case
)

qt_internal_extend_target(Multimedia CONDITION QT_FEATURE_gstreamer AND QT_FEATURE_gstreamer_photography
    LIBRARIES
        -lgstphotography-1.0#GStreamer::Photography # special case
)

qt_internal_extend_target(Multimedia CONDITION QT_FEATURE_gstreamer AND QT_FEATURE_gstreamer_gl
    LIBRARIES
        GStreamer::Gl # special case
        EGL::EGL
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
        platform/pulseaudio/qpulseaudiodevice.cpp platform/pulseaudio/qpulseaudiodevice_p.h
        platform/pulseaudio/qaudioengine_pulse.cpp platform/pulseaudio/qaudioengine_pulse_p.h
        platform/pulseaudio/qpulseaudiosource.cpp platform/pulseaudio/qpulseaudiosource_p.h
        platform/pulseaudio/qpulseaudiosink.cpp platform/pulseaudio/qpulseaudiosink_p.h
        platform/pulseaudio/qpulseaudiomediadevices.cpp platform/pulseaudio/qpulseaudiomediadevices_p.h
        platform/pulseaudio/qpulseaudiointegration.cpp platform/pulseaudio/qpulseaudiointegration_p.h
        platform/pulseaudio/qpulsehelpers.cpp platform/pulseaudio/qpulsehelpers_p.h
    LIBRARIES
        WrapPulseAudio::WrapPulseAudio
)

qt_internal_extend_target(Multimedia CONDITION QT_FEATURE_alsa
    SOURCES
        platform/alsa/qalsaaudiodevice.cpp platform/alsa/qalsaaudiodevice_p.h
        platform/alsa/qalsaaudiosource.cpp platform/alsa/qalsaaudiosource_p.h
        platform/alsa/qalsaaudiosink.cpp platform/alsa/qalsaaudiosink_p.h
        platform/alsa/qalsamediadevices.cpp platform/alsa/qalsamediadevices_p.h
        platform/alsa/qalsaintegration.cpp platform/alsa/qalsaintegration_p.h
    LIBRARIES
        ALSA::ALSA
)

qt_internal_extend_target(Multimedia CONDITION ANDROID
    SOURCES
        platform/android/audio/qandroidaudiosource.cpp platform/android/audio/qandroidaudiosource_p.h
        platform/android/audio/qandroidaudiosink.cpp platform/android/audio/qandroidaudiosink_p.h
        platform/android/audio/qandroidaudiodevice.cpp platform/android/audio/qandroidaudiodevice_p.h
        platform/android/audio/qopenslesengine.cpp platform/android/audio/qopenslesengine_p.h
        platform/android/common/qandroidaudiooutput_p.h
        platform/android/common/qandroidaudioinput.cpp platform/android/common/qandroidaudioinput_p.h
        platform/android/audio/qandroidaudiodecoder.cpp platform/android/audio/qandroidaudiodecoder_p.h
        platform/android/common/qandroidglobal_p.h
        platform/android/common/qandroidmultimediautils.cpp platform/android/common/qandroidmultimediautils_p.h
        platform/android/common/qandroidvideosink.cpp platform/android/common/qandroidvideosink_p.h
        platform/android/common/qandroidvideooutput.cpp platform/android/common/qandroidvideooutput_p.h
        platform/android/mediacapture/qandroidcamera.cpp platform/android/mediacapture/qandroidcamera_p.h
        platform/android/mediacapture/qandroidimagecapture.cpp platform/android/mediacapture/qandroidimagecapture_p.h
        platform/android/mediacapture/qandroidcamerasession.cpp platform/android/mediacapture/qandroidcamerasession_p.h
        platform/android/mediacapture/qandroidmediacapturesession.cpp platform/android/mediacapture/qandroidmediacapturesession_p.h
        platform/android/mediacapture/qandroidcapturesession.cpp platform/android/mediacapture/qandroidcapturesession_p.h
        platform/android/mediacapture/qandroidmediaencoder.cpp platform/android/mediacapture/qandroidmediaencoder_p.h
        platform/android/mediaplayer/qandroidmediaplayer.cpp platform/android/mediaplayer/qandroidmediaplayer_p.h
        platform/android/mediaplayer/qandroidmetadata.cpp platform/android/mediaplayer/qandroidmetadata_p.h
        platform/android/qandroidmediadevices.cpp platform/android/qandroidmediadevices_p.h
        platform/android/qandroidformatsinfo.cpp  platform/android/qandroidformatsinfo_p.h
        platform/android/qandroidintegration.cpp platform/android/qandroidintegration_p.h
        platform/android/wrappers/jni/androidcamera.cpp platform/android/wrappers/jni/androidcamera_p.h
        platform/android/wrappers/jni/androidmediametadataretriever.cpp platform/android/wrappers/jni/androidmediametadataretriever_p.h
        platform/android/wrappers/jni/androidmediaplayer.cpp platform/android/wrappers/jni/androidmediaplayer_p.h
        platform/android/wrappers/jni/androidmediarecorder.cpp platform/android/wrappers/jni/androidmediarecorder_p.h
        platform/android/wrappers/jni/androidmultimediautils.cpp platform/android/wrappers/jni/androidmultimediautils_p.h
        platform/android/wrappers/jni/androidsurfacetexture.cpp platform/android/wrappers/jni/androidsurfacetexture_p.h
        platform/android/wrappers/jni/androidsurfaceview.cpp platform/android/wrappers/jni/androidsurfaceview_p.h
    INCLUDE_DIRECTORIES
        platform/android/common
        platform/android/mediacapture
        platform/android/mediaplayer
        platform/android/wrappers/jni
    LIBRARIES
        OpenSLES
        mediandk
)

qt_internal_extend_target(Multimedia CONDITION WASM
    SOURCES
        platform/wasm/qwasmmediaintegration.cpp platform/wasm/qwasmmediaintegration_p.h
        platform/wasm/qwasmmediadevices.cpp platform/wasm/qwasmmediadevices_p.h
        platform/wasm/audio/qwasmaudiosource.cpp platform/wasm/audio/qwasmaudiosource_p.h
        platform/wasm/audio/qwasmaudiosink.cpp platform/wasm/audio/qwasmaudiosink_p.h
        platform/wasm/audio/qwasmaudiodevice.cpp platform/wasm/audio/qwasmaudiodevice_p.h
    PUBLIC_LIBRARIES
        openal
)

qt_internal_extend_target(Multimedia CONDITION WIN32
    LIBRARIES
        gdi32
        user32
)

qt_internal_extend_target(Multimedia CONDITION APPLE AND NOT WATCHOS
    SOURCES
        platform/darwin/audio/avfaudiodecoder.mm platform/darwin/audio/avfaudiodecoder_p.h
        platform/darwin/audio/qdarwinaudiodevice.mm platform/darwin/audio/qdarwinaudiodevice_p.h
        platform/darwin/audio/qdarwinaudiosource.mm platform/darwin/audio/qdarwinaudiosource_p.h
        platform/darwin/audio/qdarwinaudiosink.mm platform/darwin/audio/qdarwinaudiosink_p.h
        platform/darwin/audio/qcoreaudioutils.mm platform/darwin/audio/qcoreaudioutils_p.h
        platform/darwin/mediaplayer/avfdisplaylink.mm platform/darwin/mediaplayer/avfdisplaylink_p.h
        platform/darwin/mediaplayer/avfmediaplayer.mm platform/darwin/mediaplayer/avfmediaplayer_p.h
        platform/darwin/common/avfmetadata.mm platform/darwin/common/avfmetadata_p.h
        platform/darwin/mediaplayer/avfvideorenderercontrol.mm platform/darwin/mediaplayer/avfvideorenderercontrol_p.h
        platform/darwin/avfvideosink.mm platform/darwin/avfvideosink_p.h
        platform/darwin/avfvideobuffer.mm platform/darwin/avfvideobuffer_p.h
        platform/darwin/qdarwinmediadevices.mm platform/darwin/qdarwinmediadevices_p.h
        platform/darwin/qdarwinformatsinfo.mm platform/darwin/qdarwinformatsinfo_p.h
        platform/darwin/qdarwinintegration.mm platform/darwin/qdarwinintegration_p.h
    LIBRARIES
        ${FWAudioToolbox}
        ${FWCoreAudio}
        ${FWCoreFoundation}
        ${FWCoreMedia}
        ${FWCoreVideo}
        ${FWFoundation}
        ${FWMetal}
        ${FWQuartzCore}
        AVFoundation::AVFoundation
)

qt_internal_extend_target(Multimedia CONDITION MACOS AND NOT WATCHOS
    LIBRARIES
        ${FWAppKit}
        ${FWAudioUnit}
        ${FWVideoToolbox}
)

qt_internal_extend_target(Multimedia CONDITION IOS AND NOT WATCHOS
    LIBRARIES
        ${FWCoreGraphics}
        ${FWCoreVideo}
)

qt_internal_extend_target(Multimedia CONDITION APPLE AND NOT TVOS AND NOT WATCHOS
    SOURCES
        platform/darwin/camera/avfcamera.mm platform/darwin/camera/avfcamera_p.h
        platform/darwin/camera/avfcameradebug_p.h
        platform/darwin/camera/avfaudiopreviewdelegate.mm platform/darwin/camera/avfaudiopreviewdelegate_p.h
        platform/darwin/camera/avfcamerarenderer.mm platform/darwin/camera/avfcamerarenderer_p.h
        platform/darwin/camera/avfcameraservice.mm platform/darwin/camera/avfcameraservice_p.h
        platform/darwin/camera/avfcamerasession.mm platform/darwin/camera/avfcamerasession_p.h
        platform/darwin/camera/avfcamerautility.mm platform/darwin/camera/avfcamerautility_p.h
        platform/darwin/camera/avfimagecapture.mm platform/darwin/camera/avfimagecapture_p.h
)

qt_internal_extend_target(Multimedia CONDITION APPLE AND NOT TVOS AND NOT WATCHOS
    SOURCES
        platform/darwin/camera/avfmediaassetwriter.mm platform/darwin/camera/avfmediaassetwriter_p.h
        platform/darwin/camera/avfmediaencoder.mm platform/darwin/camera/avfmediaencoder_p.h
)

qt_internal_extend_target(Multimedia CONDITION APPLE AND NOT WATCHOS AND (IOS OR TVOS)
    SOURCES
        platform/darwin/audio/qcoreaudiosessionmanager.mm platform/darwin/audio/qcoreaudiosessionmanager_p.h
    LIBRARIES
        ${FWAVFoundation}
        ${FWFoundation}
)

qt_internal_extend_target(Multimedia CONDITION APPLE AND NOT IOS AND NOT TVOS AND NOT WATCHOS
    LIBRARIES
        ${FWAppKit}
        ${FWApplicationServices}
        ${FWAudioUnit}
)

qt_internal_extend_target(Multimedia CONDITION QT_FEATURE_mmrenderer
    SOURCES
        platform/qnx/audio/neutrinoserviceplugin.cpp platform/qnx/audio/neutrinoserviceplugin_p.h
        platform/qnx/audio/qqnxaudiodevice.cpp platform/qnx/audio/qqnxaudiodevice_p.h
        platform/qnx/audio/qqnxaudiosource.cpp platform/qnx/audio/qqnxaudiosource_p.h
        platform/qnx/audio/qqnxaudiosink.cpp platform/qnx/audio/qqnxaudiosink_p.h
        platform/qnx/audio/qnxaudioutils.cpp platform/qnx/audio/qnxaudioutils_p.h
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
        platform/qnx/camera/bbvideorenderercontrol.cpp platform/qnx/camera/bbvideorenderercontrol_p.h
        platform/qnx/common/windowgrabber.cpp platform/qnx/common/windowgrabber_p.h
        platform/qnx/mediaplayer/mmrenderermediaplayercontrol.cpp platform/qnx/mediaplayer/mmrenderermediaplayercontrol_p.h
        platform/qnx/mediaplayer/mmrenderermetadata.cpp platform/qnx/mediaplayer/mmrenderermetadata_p.h
        platform/qnx/mediaplayer/mmrendererplayervideorenderercontrol.cpp platform/qnx/mediaplayer/mmrendererplayervideorenderercontrol_p.h
        platform/qnx/mediaplayer/mmrendererutil.cpp platform/qnx/mediaplayer/mmrendererutil_p.h
        platform/qnx/mediaplayer/mmrenderervideowindowcontrol.cpp platform/qnx/mediaplayer/mmrenderervideowindowcontrol_p.h
        platform/qnx/mediaplayer/mmrevent_p.hread.cpp platform/qnx/mediaplayer/mmrevent_p.hread.h
        platform/qnx/mediaplayer/mmreventmediaplayercontrol.cpp platform/qnx/mediaplayer/mmreventmediaplayercontrol_p.h
        platform/qnx/qqnxmediadevices.cpp platform/qnx/qqnxmediadevices_p.h
        platform/qnx/qqnxintegration.cpp platform/qnx/qqnxintegration_p.h
    INCLUDE_DIRECTORIES
        platform/qnx/camera
        platform/qnx/common
        platform/qnx/mediaplayer
    LIBRARIES
        MMRenderer::MMRenderer
        asound
        audio_manager
        camapi
)
qt_internal_add_docs(Multimedia
    doc/qtmultimedia.qdocconf
)

qt_internal_add_shaders(Multimedia "shaders"
    SILENT
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    PREFIX
        "/qt-project.org/multimedia"
    FILES
        "shaders/vertex.vert"
        "shaders/externalsampler.vert"
        "shaders/externalsampler.frag@glsl,100es,shaders/externalsampler_gles.frag"
        "shaders/abgr.frag"
        "shaders/argb.frag"
        "shaders/rgba.frag"
        "shaders/y.frag"
        "shaders/nv12.frag"
        "shaders/nv21.frag"
        "shaders/imc2.frag"
        "shaders/imc4.frag"
        "shaders/uyvy.frag"
        "shaders/yuv_triplanar.frag"
        "shaders/yvu_triplanar.frag"
        "shaders/yuyv.frag"
        "shaders/ayuv.frag"
)

qt_internal_add_shaders(Multimedia "shaders_gl_macos"
    SILENT
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    PREFIX
        "/qt-project.org/multimedia"
    GLSL
        "120,150"
    NOHLSL
    NOMSL
    FILES
        "shaders/rectsampler.vert"
        "shaders/rectsampler_bgra.frag"
)
