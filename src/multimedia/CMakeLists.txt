# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include(Qt6MultimediaBuildInternals.cmake)
#####################################################################
## Multimedia Module:
#####################################################################

find_package(Qt6 COMPONENTS ShaderTools)


if (QT_FEATURE_gstreamer_qt_api OR FEATURE_gstreamer_qt_api)
    # TODO: implement SPI_HEADER_FILTERS for qt_internal_extend_target
    set(spi_headers_arg SPI_HEADER_FILTERS "(^|/)qgstreamer(interface|videosource)\\.h$")
endif()

qt_internal_add_module(Multimedia
    PLUGIN_TYPES multimedia
    SOURCES
        audio/qmemory_resource_tlsf_p.h
        audio/qmemory_resource_tlsf.cpp
        audio/qtaudio.cpp audio/qtaudio.h audio/qaudio.h
        audio/qaudiobuffer.cpp audio/qaudiobuffer.h
        audio/qaudiobuffer_support_p.h
        audio/qaudiodecoder.cpp audio/qaudiodecoder.h audio/qaudiodecoder_p.h
        audio/qaudiodevice.cpp audio/qaudiodevice.h audio/qaudiodevice_p.h
        audio/qaudioinput.cpp audio/qaudioinput.h
        audio/qaudiobufferinput.cpp audio/qaudiobufferinput.h
        audio/qaudiobufferoutput.cpp audio/qaudiobufferoutput.h audio/qaudiobufferoutput_p.h
        audio/qaudiooutput.cpp audio/qaudiooutput.h
        audio/qaudioformat.cpp audio/qaudioformat.h audio/qaudioformat_p.h
        audio/qaudiohelpers.cpp audio/qaudiohelpers_p.h
        audio/qaudioringbuffer_p.h
        audio/qaudiosource.cpp audio/qaudiosource.h
        audio/qaudiosink.cpp audio/qaudiosink.h
        audio/qaudiosystem.cpp audio/qaudiosystem_p.h
        audio/qaudiosystem_platform_stream_support.cpp audio/qaudiosystem_platform_stream_support_p.h
        audio/qaudio_platform_implementation_support_p.h
        audio/qaudio_qiodevice_support_p.h
        audio/qaudio_qspan_support_p.h
        audio/qsamplecache_p.cpp audio/qsamplecache_p.h
        audio/qsoundeffect.cpp audio/qsoundeffect.h
        audio/qsoundeffectwithplayer.cpp audio/qsoundeffectwithplayer_p.h
        audio/qsoundeffectsynchronous.cpp audio/qsoundeffectsynchronous_p.h
        audio/qsoundeffect_p.h
        audio/qrtaudioengine.cpp audio/qrtaudioengine_p.h
        audio/qwavedecoder.cpp audio/qwavedecoder.h
        camera/qcamera.cpp camera/qcamera.h camera/qcamera_p.h
        camera/qcameradevice.cpp camera/qcameradevice.h camera/qcameradevice_p.h
        camera/qimagecapture.cpp camera/qimagecapture.h
        compat/removed_api.cpp
        platform/qplatformaudiodecoder.cpp platform/qplatformaudiodecoder_p.h
        platform/qplatformaudioinput_p.h
        platform/qplatformaudiooutput_p.h
        platform/qplatformaudioresampler_p.h
        platform/qplatformcamera.cpp platform/qplatformcamera_p.h
        platform/qplatformcapturablewindows_p.h
        platform/qplatformimagecapture.cpp platform/qplatformimagecapture_p.h
        platform/qplatformmediacapture.cpp platform/qplatformmediacapture_p.h
        platform/qplatformaudiodevices.cpp platform/qplatformaudiodevices_p.h
        platform/qplatformmediaformatinfo.cpp  platform/qplatformmediaformatinfo_p.h
        platform/qplatformmediaintegration.cpp platform/qplatformmediaintegration_p.h
        platform/qplatformmediaplayer.cpp platform/qplatformmediaplayer_p.h
        platform/qplatformmediaplugin.cpp platform/qplatformmediaplugin_p.h
        platform/qplatformmediarecorder.cpp platform/qplatformmediarecorder_p.h
        platform/qplatformsurfacecapture.cpp platform/qplatformsurfacecapture_p.h
        platform/qplatformvideodevices.cpp platform/qplatformvideodevices_p.h
        platform/qplatformvideosink.cpp platform/qplatformvideosink_p.h
        platform/qplatformvideosource.cpp platform/qplatformvideosource_p.h
        platform/qplatformvideoframeinput.cpp platform/qplatformvideoframeinput_p.h
        platform/qplatformaudiobufferinput.cpp platform/qplatformaudiobufferinput_p.h
        playback/qmediaplayer.cpp playback/qmediaplayer.h playback/qmediaplayer_p.h
        playback/qplaybackoptions.cpp playback/qplaybackoptions.h
        qmultimedia_enum_to_string_converter_p.h
        qmediadevices.cpp qmediadevices.h
        qmediaformat.cpp  qmediaformat.h
        qmediametadata.cpp qmediametadata.h qmediametadata_p.h
        qmediastoragelocation.cpp qmediastoragelocation_p.h
        qmediatimerange.cpp qmediatimerange.h
        qmultimediautils.cpp qmultimediautils_p.h
        qmultimedia_ranges_p.h
        qvideotransformation.cpp qvideotransformation_p.h
        qmediaframeinput.cpp qmediaframeinput_p.h
        qtmultimediaglobal.h qtmultimediaglobal_p.h
        qmultimedia_source_resolver.cpp qmultimedia_source_resolver_p.h
        qerrorinfo_p.h
        qcachedvalue_p.h
        qmediainputencoderinterface_p.h
        qrhivaluemapper.cpp  qrhivaluemapper_p.h
        qthreadlocalrhi.cpp  qthreadlocalrhi_p.h
        qsurfacecapturegrabber.cpp qsurfacecapturegrabber_p.h
        qsharedhandle_p.h
        qtaggedtime_p.h
        recording/qmediacapturesession.cpp recording/qmediacapturesession.h recording/qmediacapturesession_p.h
        recording/qmediarecorder.cpp recording/qmediarecorder.h recording/qmediarecorder_p.h
        recording/qscreencapture.cpp recording/qscreencapture.h
        recording/qwindowcapture.cpp recording/qwindowcapture.h recording/qwindowcapture_p.h
        recording/qcapturablewindow.cpp recording/qcapturablewindow.h recording/qcapturablewindow_p.h
        recording/qvideoframeinput.cpp recording/qvideoframeinput.h
        support/qiteratorfacade_p.h
        support/qautoresetevent_p.h
        support/qtmm_alignment_support_p.h
        support/qtmm_pmr_p.h
        support/qtmm_rtsan_support_p.h
        video/qabstractvideobuffer.cpp video/qabstractvideobuffer.h
        video/qhwvideobuffer.cpp video/qhwvideobuffer_p.h
        video/qimagevideobuffer.cpp video/qimagevideobuffer_p.h
        video/qmemoryvideobuffer.cpp video/qmemoryvideobuffer_p.h
        video/qmultimedia_drm_support_p.h
        video/qtvideo.cpp video/qtvideo.h
        video/qvideoframe.cpp video/qvideoframe.h video/qvideoframe_p.h
        video/qvideoframeconversionhelper.cpp video/qvideoframeconversionhelper_p.h
        video/qvideoframeconverter.cpp video/qvideoframeconverter_p.h
        video/qvideoframeformat.cpp video/qvideoframeformat.h
        video/qvideoframetexturefromsource.cpp video/qvideoframetexturefromsource_p.h
        video/qvideoframetexturepool.cpp video/qvideoframetexturepool_p.h
        video/qvideooutputorientationhandler.cpp video/qvideooutputorientationhandler_p.h
        video/qvideosink.cpp video/qvideosink.h
        video/qvideotexturehelper.cpp video/qvideotexturehelper_p.h
        video/qvideowindow.cpp video/qvideowindow_p.h
        shaders/qrhitextureformats_p.h
    PRECOMPILED_HEADER
        qtmultimedia_pch_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::BundledTLSF
        Qt::Concurrent
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
        Qt::GuiPrivate
    NO_PCH_SOURCES
        compat/removed_api.cpp
    ${spi_headers_arg}
)

qt_internal_extend_target(Multimedia CONDITION QT_FEATURE_gstreamer_qt_api
    SOURCES
        spi/qgstreamervideosource.cpp spi/qgstreamervideosource.h spi/qgstreamervideosource_p.h
        spi/qgstreamerinterface.cpp spi/qgstreamerinterface.h
        platform/qgstreamer_platformspecificinterface_p.h
)


# dr_wav preparation
qt_internal_extend_target(Multimedia
    SOURCES
        audio/qaudio_dr_wav.cpp
        ../3rdparty/dr_libs/dr_wav.h
    SYSTEM_INCLUDE_DIRECTORIES
        ../3rdparty/dr_libs
    DEFINES
        DR_WAV_NO_STDIO
    ATTRIBUTION_FILE_DIR_PATHS
        ../3rdparty/dr_libs
)

qt_internal_extend_target(Multimedia CONDITION QT_FEATURE_network
    PUBLIC_LIBRARIES
        Qt::Network
)

qt_internal_extend_target(Multimedia
    CONDITION (LINUX OR ANDROID) AND QT_FEATURE_library
    SOURCES qsymbolsresolveutils.cpp qsymbolsresolveutils_p.h)

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

qt_internal_extend_target(Multimedia CONDITION QT_FEATURE_alsa
    SOURCES
        alsa/qalsaaudiodevice.cpp alsa/qalsaaudiodevice_p.h
        alsa/qalsaaudiosource.cpp alsa/qalsaaudiosource_p.h
        alsa/qalsaaudiosink.cpp alsa/qalsaaudiosink_p.h
        alsa/qalsaaudiodevices.cpp alsa/qalsaaudiodevices_p.h
    LIBRARIES
        ALSA::ALSA
)

if (QT_FEATURE_pipewire)
    qt_internal_extend_target(Multimedia
        SYSTEM_INCLUDE_DIRECTORIES
            "${PipeWire_INCLUDE_DIRS};${Spa_INCLUDE_DIRS}"
        SOURCES
            pipewire/qpipewire_async_support.cpp       pipewire/qpipewire_async_support_p.h
            pipewire/qpipewire_audiocontextmanager.cpp pipewire/qpipewire_audiocontextmanager_p.h
            pipewire/qpipewire_audiodevice.cpp         pipewire/qpipewire_audiodevice_p.h
            pipewire/qpipewire_audiodevicemonitor.cpp  pipewire/qpipewire_audiodevicemonitor_p.h
            pipewire/qpipewire_audiodevices.cpp        pipewire/qpipewire_audiodevices_p.h
            pipewire/qpipewire_audiosink.cpp           pipewire/qpipewire_audiosink_p.h
            pipewire/qpipewire_audiosource.cpp         pipewire/qpipewire_audiosource_p.h
            pipewire/qpipewire_audiostream.cpp         pipewire/qpipewire_audiostream_p.h
            pipewire/qpipewire_instance.cpp            pipewire/qpipewire_instance_p.h
            pipewire/qpipewire_propertydict.cpp        pipewire/qpipewire_propertydict_p.h
            pipewire/qpipewire_registry_support.cpp    pipewire/qpipewire_registry_support_p.h
            pipewire/qpipewire_spa_compat_p.h
            pipewire/qpipewire_spa_pod_support.cpp     pipewire/qpipewire_spa_pod_support_p.h
            pipewire/qpipewire_spa_pod_parser_support_p.h
            pipewire/qpipewire_videoformat_support.cpp pipewire/qpipewire_videoformat_support_p.h
            pipewire/qpipewire_support.cpp             pipewire/qpipewire_support_p.h
    )

    if(QT_FEATURE_pipewire_symbolloader)
        qt_internal_extend_target(Multimedia
            SOURCES
                pipewire/qpipewire_symbolloader.cpp pipewire/qpipewire_symbolloader_p.h
        )
    else()
        qt_internal_extend_target(Multimedia
            LIBRARIES
                ${PipeWire_LIBRARIES}
        )
    endif()


    if (QT_FEATURE_pipewire_screencapture)
        qt_internal_extend_target(Multimedia
            SOURCES
                pipewire/qpipewire_screencapture.cpp       pipewire/qpipewire_screencapture_p.h
                pipewire/qpipewire_screencapturehelper.cpp pipewire/qpipewire_screencapturehelper_p.h
            LIBRARIES
                Qt::DBus
        )
    endif()

    # pipewire macros emit -Wsign-compare warnings
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag(QT_MM_HAS_WSIGN_COMPARE -Wsign-compare)
    if (QT_MM_HAS_WSIGN_COMPARE)
        set_source_files_properties(
            pipewire/qpipewire_async_support.cpp
            pipewire/qpipewire_audiocontextmanager.cpp
            pipewire/qpipewire_audiodevicemonitor.cpp
            pipewire/qpipewire_audiodevices.cpp
            pipewire/qpipewire_audiosink.cpp
            pipewire/qpipewire_instance.cpp
            pipewire/qpipewire_screencapture.cpp
            pipewire/qpipewire_screencapturehelper.cpp
            PROPERTIES COMPILE_FLAGS -Wno-sign-compare
        )
    endif()

    # pipewire macros emit -Wunused-value warning
    check_cxx_compiler_flag(QT_MM_HAS_UNUSED_VALUE -Wunused-value)
    if (QT_MM_HAS_UNUSED_VALUE)
        set_source_files_properties(
            pipewire/qpipewire_async_support.cpp
            PROPERTIES COMPILE_FLAGS -Wno-unused-value
        )
    endif()
endif()


qt_internal_extend_target(Multimedia CONDITION QT_FEATURE_pulseaudio
    SOURCES
        pulseaudio/qpulseaudio_contextmanager.cpp pulseaudio/qpulseaudio_contextmanager_p.h
        pulseaudio/qpulseaudiosource.cpp pulseaudio/qpulseaudiosource_p.h
        pulseaudio/qpulseaudiosink.cpp pulseaudio/qpulseaudiosink_p.h
        pulseaudio/qpulsehelpers.cpp pulseaudio/qpulsehelpers_p.h
        pulseaudio/qpulseaudiodevices.cpp pulseaudio/qpulseaudiodevices_p.h
        pulseaudio/qpulseaudiodevice_p.h
    LIBRARIES
        WrapPulseAudio::WrapPulseAudio
)

qt_internal_extend_target(Multimedia CONDITION ANDROID
    SOURCES
        android/qandroidaudiosource.cpp android/qandroidaudiosource_p.h
        android/qandroidaudiosink.cpp android/qandroidaudiosink_p.h
        android/qandroidaudiodevice.cpp android/qandroidaudiodevice_p.h
        android/qandroidaudiodevices.cpp android/qandroidaudiodevices_p.h
        android/qandroidaudiojnitypes_p.h
        android/qandroidaudioutil.cpp android/qandroidaudioutil_p.h
        android/qaaudiostream.cpp android/qaaudiostream_p.h
    LIBRARIES
        aaudio
)

if(ANDROID)
    set_property(TARGET Multimedia APPEND PROPERTY QT_ANDROID_BUNDLED_JAR_DEPENDENCIES
        jar/Qt${QtMultimedia_VERSION_MAJOR}AndroidMultimedia.jar
    )
endif()

qt_internal_extend_target(Multimedia CONDITION QT_FEATURE_ohaudio
    SOURCES
        ohos/qohaudiostream.cpp ohos/qohaudiostream_p.h
        ohos/qohosaudiodevice.cpp ohos/qohosaudiodevice_p.h
        ohos/qohosaudiodevices.cpp ohos/qohosaudiodevices_p.h
        ohos/qohosaudiosink.cpp ohos/qohosaudiosink_p.h
        ohos/qohosaudiosource.cpp ohos/qohosaudiosource_p.h
    LIBRARIES
        ohaudio
)

if(OHOS)
    qt_internal_add_harmonyos_permission(Multimedia
        NAME ohos.permission.MICROPHONE
        REASON "Used to record audio."
    )
    qt_internal_add_harmonyos_permission(Multimedia
        NAME ohos.permission.CAMERA
        REASON "Used to capture camera images and video."
    )
endif()

qt_internal_extend_target(Multimedia CONDITION LINUX OR ANDROID OR OHOS
    SOURCES
        support/qautoresetevent_linux_p.h support/qautoresetevent_linux.cpp
)

qt_internal_extend_target(Multimedia CONDITION APPLE
    SOURCES
        support/qautoresetevent_kqueue_p.h support/qautoresetevent_kqueue.cpp
)


qt_internal_extend_target(Multimedia CONDITION UNIX
    SOURCES
        support/qautoresetevent_pipe_p.h support/qautoresetevent_pipe.cpp
)

qt_internal_extend_target(Multimedia CONDITION WIN32
    SOURCES
        support/qautoresetevent_win32_p.h support/qautoresetevent_win32.cpp
)


qt_internal_extend_target(Multimedia CONDITION APPLE
    SOURCES
        coreaudio/qcoreaudiodevice.cpp coreaudio/qcoreaudiodevice_p.h
        coreaudio/qcoreaaudiosource.mm coreaudio/qcoreaaudiosource_p.h
        coreaudio/qcoreaaudiosink.mm coreaudio/qcoreaaudiosink_p.h
        coreaudio/qcoreaudioutils.cpp coreaudio/qcoreaudioutils_p.h
        coreaudio/qcoreaudiodevices.mm coreaudio/qcoreaudiodevices_p.h
        darwin/qapple_utils_p.h
        darwin/qavfhelpers.mm darwin/qavfhelpers_p.h
    LIBRARIES
        ${FWCoreAudio}
        ${FWAudioToolbox}
        ${FWCoreVideo}
)

qt_internal_extend_target(Multimedia CONDITION MACOS OR IOS
    SOURCES
        darwin/qdarwinintegrationfactory_p.h darwin/qdarwinintegrationfactory.mm
        darwin/qavfcameradebug_p.h
        darwin/qavfcamerautility_p.h darwin/qavfcamerautility.mm
        darwin/qavfvideodevices.mm darwin/qavfvideodevices_p.h
        darwin/qavfcamerabase.mm darwin/qavfcamerabase_p.h
    LIBRARIES
        ${FWCoreMedia}
        ${FWAVFoundation}
)

qt_internal_extend_target(Multimedia CONDITION MACOS
    SOURCES
        coreaudio/qmacosaudiodatautils_p.h coreaudio/qmacosaudiodatautils.cpp
)

qt_internal_extend_target(Multimedia CONDITION IOS OR TVOS OR VISIONOS
    SOURCES
        coreaudio/qcoreaudiosessionmanager.mm coreaudio/qcoreaudiosessionmanager_p.h
    LIBRARIES
        ${FWAVFoundation}
        ${FWFoundation}
)

qt_internal_extend_target(Multimedia CONDITION QT_FEATURE_qnx_io_audio
    SOURCES
        qnx/qqnxaudiodevice.cpp qnx/qqnxaudiodevice_p.h
        qnx/qqnxaudiosink.cpp qnx/qqnxaudiosink_p.h
        qnx/qqnxaudiosource.cpp qnx/qqnxaudiosource_p.h
        qnx/qqnxaudioutils.cpp qnx/qqnxaudioutils_p.h
        qnx/qqnxaudiodevices.cpp qnx/qqnxaudiodevices_p.h
    DEFINES
        QT_MM_QNX_IO_AUDIO
    INCLUDE_DIRECTORIES
        qnx
    LIBRARIES
        asound
)

qt_internal_extend_target(Multimedia CONDITION QT_FEATURE_qnx_io_snd
    SOURCES
        qnx-iosnd/qqnxsndaudiodevice.cpp qnx-iosnd/qqnxsndaudiodevice_p.h
        qnx-iosnd/qqnxsndaudiosink.cpp qnx-iosnd/qqnxsndaudiosink_p.h
        qnx-iosnd/qqnxsndaudiosource.cpp qnx-iosnd/qqnxsndaudiosource_p.h
        qnx-iosnd/qqnxsndaudiodevices.cpp qnx-iosnd/qqnxsndaudiodevices_p.h
        qnx-iosnd/qqnxsndhelpers.cpp qnx-iosnd/qqnxsndhelpers_p.h
    DEFINES
        QT_MM_QNX_IO_SND
    INCLUDE_DIRECTORIES
        qnx-iosnd
    LIBRARIES
        asound
)

qt_internal_extend_target(Multimedia CONDITION WIN32
    SOURCES
        windows/qcomtaskresource_p.h
        windows/qwindows_propertystore.cpp windows/qwindows_propertystore_p.h
        windows/qwindows_scopedpropvariant_p.h
        windows/qwindows_wasapi_warmup_client.cpp windows/qwindows_wasapi_warmup_client_p.h
        windows/qwindowsaudiodevice.cpp windows/qwindowsaudiodevice_p.h
        windows/qwindowsaudiodevices.cpp windows/qwindowsaudiodevices_p.h
        windows/qwindowsaudiosink.cpp windows/qwindowsaudiosink_p.h
        windows/qwindowsaudiosource.cpp windows/qwindowsaudiosource_p.h
        windows/qwindowsaudioutils.cpp windows/qwindowsaudioutils_p.h
        windows/qwindowsmediafoundation.cpp windows/qwindowsmediafoundation_p.h
        windows/qwindowsmultimediautils.cpp windows/qwindowsmultimediautils_p.h
        windows/qwindowsresampler.cpp windows/qwindowsresampler_p.h
        windows/qwindowsvideodevices.cpp windows/qwindowsvideodevices_p.h
        windows/qwmf_support.cpp windows/qwmf_support_p.h
    LIBRARIES
        avrt
        ksuser
        propsys
        uuid
    NO_UNITY_BUILD_SOURCES
        # audioclient.h causes problems with mingw
        windows/qwindows_wasapi_warmup_client.cpp
        windows/qwindowsaudiodevice.cpp
        windows/qwindowsaudiodevices.cpp
        windows/qwindowsaudiosink.cpp
        windows/qwindowsaudiosource.cpp
        windows/qwindowsaudioutils.cpp
    )

set(source_with_init_guid
    windows/qwindows_wasapi_warmup_client.cpp
    windows/qwindowsmuldimediautils.cpp
    windows/qwindowsaudiodevices.cpp
    windows/qwindowsvideodevices.cpp
)

set_source_files_properties(${source_with_init_guid} PROPERTIES
    COMPILE_DEFINITIONS     INITGUID
    SKIP_PRECOMPILE_HEADERS TRUE
)


qt_internal_extend_target(Multimedia CONDITION WASM
    SOURCES
        wasm/qwasmmediadevices.cpp wasm/qwasmmediadevices_p.h
        wasm/qwasmwebaudiosource.cpp wasm/qwasmwebaudiosource_p.h
        wasm/qwasmwebaudiosink.cpp wasm/qwasmwebaudiosink_p.h
        wasm/qwasmaudiodevice.cpp wasm/qwasmaudiodevice_p.h
        wasm/qwasmjs.cpp wasm/qwasmjs_p.h
    INCLUDE_DIRECTORIES
        wasm
)

if(WASM AND QT_FEATURE_thread)
    target_link_options(Multimedia PUBLIC -sAUDIO_WORKLET=1 -sWASM_WORKERS=1)
    target_compile_options(Multimedia PUBLIC -sWASM_WORKERS=1)
endif()

set(VIDEO_VERTEX_SHADERS
    "shaders/vertex.vert"
    "shaders/externalsampler.vert"
)

set(VIDEO_SHADERS
    "shaders/externalsampler.frag@glsl,100es,shaders/externalsampler_gles.frag@glsl,300es,shaders/externalsampler_gles3.frag"
    "shaders/abgr.frag"
    "shaders/argb.frag"
    "shaders/rgba.frag"
    "shaders/bgra.frag"
    "shaders/y.frag"
    "shaders/y16.frag"
    "shaders/nv12.frag"
    "shaders/nv21.frag"
    "shaders/p016.frag"
    "shaders/imc2.frag"
    "shaders/imc4.frag"
    "shaders/uyvy.frag"
    "shaders/yuv_triplanar.frag"
    "shaders/yuv_triplanar_p10.frag"
    "shaders/yvu_triplanar.frag"
    "shaders/yuyv.frag"
    "shaders/ayuv.frag"
    "shaders/nv12_bt2020_pq.frag"
    "shaders/nv12_bt2020_hlg.frag"
)

set(VIDEO_SHADERS_UTILS
    "shaders/texturecomponent.glsl"
    "shaders/colortransfer.glsl"
    "shaders/colorconvert.glsl"
    "shaders/uniformbuffer.glsl"
    "shaders/hdrtonemapper.glsl"
)

set_source_files_properties(${VIDEO_SHADERS_UTILS} PROPERTIES HEADER_FILE_ONLY ON)
target_sources(Multimedia PRIVATE ${VIDEO_SHADERS_UTILS})

qt_internal_add_shaders(Multimedia "qtmultimedia_shaders"
    SILENT
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    PREFIX
        "/qt-project.org/multimedia"
    FILES
        ${VIDEO_VERTEX_SHADERS}
        ${VIDEO_SHADERS}
)

string(REPLACE ".frag" "_linear.frag.qsb" LINEAR_VIDEO_SHADERS "${VIDEO_SHADERS}")

qt_internal_add_shaders(Multimedia "qtmultimedia_shaders_linear"
    SILENT
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    PREFIX
        "/qt-project.org/multimedia"
    FILES
        ${VIDEO_SHADERS}
    OUTPUTS
        ${LINEAR_VIDEO_SHADERS}
    DEFINES
        QMM_OUTPUTSURFACE_LINEAR
)

qt_internal_add_shaders(Multimedia "qtmultimedia_shaders_gl_macos"
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

qt_internal_add_shaders(Multimedia "qtmultimedia_shaders_gl_macos_linear"
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
        "shaders/rectsampler_bgra.frag"
    OUTPUTS
        "shaders/rectsampler_bgra_linear.frag.qsb"
)

if(DEFINED QT_DEFAULT_MEDIA_BACKEND)
    target_compile_definitions(Multimedia
        PRIVATE QT_DEFAULT_MEDIA_BACKEND="${QT_DEFAULT_MEDIA_BACKEND}")
endif()

if(QT_FEATURE_doc_snippets)
    add_subdirectory(doc/snippets)
endif()

# Install public QMake support files.
qt_path_join(ios_prf_install_dir "${QT_INSTALL_DIR}" "${INSTALL_MKSPECSDIR}" features ios)
qt_copy_or_install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/../../mkspecs/features/ios/add_ios_ffmpeg_libraries.prf"
    DESTINATION ${ios_prf_install_dir}
)
