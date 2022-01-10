# Generated from multimedia.pro.

#####################################################################
## Multimedia Module:
#####################################################################

find_package(Qt6 COMPONENTS ShaderTools)

qt_internal_add_module(Multimedia
    PLUGIN_TYPES multimedia
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
        platform/qplatformmediaplugin_p.h
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
    GENERATE_CPP_EXPORTS
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

qt_internal_extend_target(Multimedia CONDITION WIN32
    LIBRARIES
        gdi32
        user32
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
