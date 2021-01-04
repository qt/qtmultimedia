# Generated from multimedia.pro.

#####################################################################
## Multimedia Module:
#####################################################################

qt_add_module(Multimedia
    PLUGIN_TYPES mediaservice audio video/bufferpool video/gstvideorenderer video/videonode playlistformats
    SOURCES
        audio/qaudio.cpp audio/qaudio.h
        audio/qaudiobuffer.cpp audio/qaudiobuffer.h audio/qaudiobuffer_p.h
        audio/qaudiodecoder.cpp audio/qaudiodecoder.h
        audio/qaudiodevicefactory.cpp audio/qaudiodevicefactory_p.h
        audio/qaudiodeviceinfo.cpp audio/qaudiodeviceinfo.h
        audio/qaudioformat.cpp audio/qaudioformat.h
        audio/qaudiohelpers.cpp audio/qaudiohelpers_p.h
        audio/qaudioinput.cpp audio/qaudioinput.h
        audio/qaudiooutput.cpp audio/qaudiooutput.h
        audio/qaudioprobe.cpp audio/qaudioprobe.h
        audio/qaudiosystem.cpp audio/qaudiosystem_p.h
        audio/qsamplecache_p.cpp audio/qsamplecache_p.h
        audio/qsoundeffect.cpp audio/qsoundeffect.h
        audio/qsoundeffect_qaudio_p.cpp audio/qsoundeffect_qaudio_p.h
        audio/qwavedecoder_p.cpp audio/qwavedecoder_p.h
        camera/qcamera.cpp camera/qcamera.h camera/qcamera_p.h
        camera/qcameraexposure.cpp camera/qcameraexposure.h
        camera/qcamerafocus.cpp camera/qcamerafocus.h
        camera/qcameraimagecapture.cpp camera/qcameraimagecapture.h
        camera/qcameraimageprocessing.cpp camera/qcameraimageprocessing.h
        camera/qcamerainfo.cpp camera/qcamerainfo.h
        camera/qcameraviewfindersettings.cpp camera/qcameraviewfindersettings.h
        controls/qaudiodecodercontrol.cpp controls/qaudiodecodercontrol.h
        controls/qaudioencodersettingscontrol.cpp controls/qaudioencodersettingscontrol.h
        controls/qaudioinputselectorcontrol.cpp controls/qaudioinputselectorcontrol.h
        controls/qaudiooutputselectorcontrol.cpp controls/qaudiooutputselectorcontrol.h
        controls/qcameracapturebufferformatcontrol.cpp controls/qcameracapturebufferformatcontrol.h
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
        playback/qmediacontent.cpp playback/qmediacontent.h
        playback/qmediaplayer.cpp playback/qmediaplayer.h
        playback/qmediaplaylist.cpp playback/qmediaplaylist.h playback/qmediaplaylist_p.h
        playback/qplaylistfileparser.cpp playback/qplaylistfileparser_p.h
        qmediabindableinterface.cpp qmediabindableinterface.h
        qmediaenumdebug.h
        qmediametadata.cpp qmediametadata.h
        qmediaobject.cpp qmediaobject.h qmediaobject_p.h
        qmediapluginloader.cpp qmediapluginloader_p.h
        qmediaservice.cpp qmediaservice.h qmediaservice_p.h
        qmediaserviceprovider.cpp qmediaserviceprovider_p.h
        qmediaserviceproviderplugin.h
        qmediastoragelocation.cpp qmediastoragelocation_p.h
        qmediatimerange.cpp qmediatimerange.h
        qmultimedia.cpp qmultimedia.h
        qmultimediautils.cpp qmultimediautils_p.h
        qtmultimediaglobal.h qtmultimediaglobal_p.h
        recording/qaudiorecorder.cpp recording/qaudiorecorder.h
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


qt_add_simd_part(Multimedia SIMD sse2
    SOURCES
        video/qvideoframeconversionhelper_sse2.cpp
)

qt_add_simd_part(Multimedia SIMD ssse3
    SOURCES
        video/qvideoframeconversionhelper_ssse3.cpp
)

qt_add_simd_part(Multimedia SIMD avx2
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

if(ANDROID AND (QT_FEATURE_gstreamer))
    set_property(TARGET Multimedia APPEND PROPERTY QT_ANDROID_LIB_DEPENDENCIES
        plugins/mediaservice/libgstaudiodecoder.so
        plugins/mediaservice/libgstcamerabin.so
        plugins/mediaservice/libgstmediacapture.so
        plugins/mediaservice/libgstmediaplayer.so
    )
endif()

qt_extend_target(Multimedia CONDITION WIN32
    SOURCES
        audio/windows/qwindowsaudiodeviceinfo.cpp audio/windows/qwindowsaudiodeviceinfo_p.h
        audio/windows/qwindowsaudioinput.cpp audio/windows/qwindowsaudioinput_p.h
        audio/windows/qwindowsaudiointerface.cpp audio/windows/qwindowsaudiointerface_p.h
        audio/windows/qwindowsaudiooutput.cpp audio/windows/qwindowsaudiooutput_p.h
        audio/windows/qwindowsaudioutils.cpp audio/windows/qwindowsaudioutils_p.h
    LIBRARIES
        uuid
    PUBLIC_LIBRARIES
        ole32
        oleaut32
        strmiids
        winmm
)

qt_extend_target(Multimedia CONDITION QT_FEATURE_pulseaudio
    SOURCES
        audio/pulseaudio/qaudiodeviceinfo_pulse.cpp audio/pulseaudio/qaudiodeviceinfo_pulse_p.h
        audio/pulseaudio/qaudioengine_pulse.cpp audio/pulseaudio/qaudioengine_pulse_p.h
        audio/pulseaudio/qaudioinput_pulse.cpp audio/pulseaudio/qaudioinput_pulse_p.h
        audio/pulseaudio/qaudiointerface_pulse.cpp audio/pulseaudio/qaudiointerface_pulse_p.h
        audio/pulseaudio/qaudiooutput_pulse.cpp audio/pulseaudio/qaudiooutput_pulse_p.h
        audio/pulseaudio/qpulsehelpers.cpp audio/pulseaudio/qpulsehelpers_p.h
    LIBRARIES
        pulseaudio
    PRIVATE_MODULE_INTERFACE
        pulseaudio
)

qt_extend_target(Multimedia CONDITION ANDROID
    SOURCES
        audio/opensles/qopenslesaudioinput.cpp audio/opensles/qopenslesaudioinput_p.h
        audio/opensles/qopenslesaudiooutput.cpp audio/opensles/qopenslesaudiooutput_p.h
        audio/opensles/qopenslesdeviceinfo.cpp audio/opensles/qopenslesdeviceinfo_p.h
        audio/opensles/qopenslesengine.cpp audio/opensles/qopenslesengine_p.h
        audio/opensles/qopenslesinterface.cpp audio/opensles/qopenslesinterface_p.h
    PUBLIC_LIBRARIES
        OpenSLES
)
qt_internal_find_apple_system_framework(FWCoreAudio CoreAudio) # special case

qt_internal_extend_target(Multimedia CONDITION APPLE AND NOT WATCHOS
    SOURCES
        audio/coreaudio/qcoreaudiodeviceinfo.mm audio/coreaudio/qcoreaudiodeviceinfo_p.h
        audio/coreaudio/qcoreaudioinput.mm audio/coreaudio/qcoreaudioinput_p.h
        audio/coreaudio/qcoreaudiointerface.mm audio/coreaudio/qcoreaudiointerface_p.h
        audio/coreaudio/qcoreaudiooutput.mm audio/coreaudio/qcoreaudiooutput_p.h
        audio/coreaudio/qcoreaudioutils.mm audio/coreaudio/qcoreaudioutils_p.h
    PUBLIC_LIBRARIES
        ${FWAudioToolbox}
        ${FWCoreAudio}
)

qt_internal_extend_target(Multimedia CONDITION APPLE AND NOT WATCHOS AND (IOS OR TVOS)
    SOURCES
        qcoreaudiosessionmanager.mm qcoreaudiosessionmanager_p.h
    PUBLIC_LIBRARIES
        ${FWAVFoundation}
        ${FWFoundation}
)

qt_internal_extend_target(Multimedia CONDITION APPLE AND NOT IOS AND NOT TVOS AND NOT WATCHOS
    PUBLIC_LIBRARIES
        ${FWApplicationServices}
        ${FWAudioUnit}
)

qt_extend_target(Multimedia CONDITION QNX
    SOURCES
        audio/qnx/qnxaudiodeviceinfo.cpp audio/qnx/qnxaudiodeviceinfo_p.h
        audio/qnx/qnxaudioinput.cpp audio/qnx/qnxaudioinput_p.h
        audio/qnx/qnxaudiointerface.cpp audio/qnx/qnxaudiointerface_p.h
        audio/qnx/qnxaudiooutput.cpp audio/qnx/qnxaudiooutput_p.h
        audio/qnx/qnxaudioutils.cpp audio/qnx/qnxaudioutils_p.h
    PUBLIC_LIBRARIES
        asound
)

qt_extend_target(Multimedia CONDITION QT_FEATURE_alsa
    SOURCES
        audio/alsa/qalsaaudiodeviceinfo.cpp audio/alsa/qalsaaudiodeviceinfo_p.h
        audio/alsa/qalsaaudioinput.cpp audio/alsa/qalsaaudioinput_p.h
        audio/alsa/qalsaaudiooutput.cpp audio/alsa/qalsaaudiooutput_p.h
        audio/alsa/qalsainterface.cpp audio/alsa/qalsainterface_p.h
    LIBRARIES
        alsa
)
qt_add_docs(Multimedia
    doc/qtmultimedia.qdocconf
)
