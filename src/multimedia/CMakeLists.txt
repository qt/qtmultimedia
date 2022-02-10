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
        video/qvideoframeconverter.cpp video/qvideoframeconverter_p.h
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

qt_internal_add_simd_part(Multimedia SIMD arch_haswell
    SOURCES
        video/qvideoframeconversionhelper_avx2.cpp
    EXCLUDE_OSX_ARCHITECTURES
        arm64
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
        "shaders/bgra.frag"
        "shaders/y.frag"
        "shaders/nv12.frag"
        "shaders/nv21.frag"
        "shaders/imc2.frag"
        "shaders/imc4.frag"
        "shaders/uyvy.frag"
        "shaders/yuv_triplanar.frag"
        "shaders/yuv_triplanar_p10.frag"
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
