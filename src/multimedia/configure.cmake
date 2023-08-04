# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause



#### Inputs



#### Libraries

qt_find_package(ALSA PROVIDED_TARGETS ALSA::ALSA MODULE_NAME multimedia QMAKE_LIB alsa)
qt_find_package(AVFoundation PROVIDED_TARGETS AVFoundation::AVFoundation MODULE_NAME multimedia QMAKE_LIB avfoundation)
qt_find_package(GStreamer PROVIDED_TARGETS GStreamer::GStreamer MODULE_NAME multimedia QMAKE_LIB gstreamer_1_0)
qt_find_package(GStreamer COMPONENTS App PROVIDED_TARGETS GStreamer::App MODULE_NAME multimedia QMAKE_LIB gstreamer_app_1_0)
qt_add_qmake_lib_dependency(gstreamer_app_1_0 gstreamer_1_0)
qt_find_package(GStreamer OPTIONAL_COMPONENTS Photography PROVIDED_TARGETS GStreamer::Photography MODULE_NAME multimedia QMAKE_LIB gstreamer_photography_1_0) # special case
qt_add_qmake_lib_dependency(gstreamer_photography_1_0 gstreamer_1_0)
qt_find_package(GStreamer OPTIONAL_COMPONENTS Gl PROVIDED_TARGETS GStreamer::Gl MODULE_NAME multimedia QMAKE_LIB gstreamer_gl_1_0) # special case
qt_add_qmake_lib_dependency(gstreamer_gl_1_0 gstreamer_1_0)
qt_find_package(MMRendererCore PROVIDED_TARGETS MMRendererCore::MMRendererCore MODULE_NAME multimedia QMAKE_LIB mmrndcore)
qt_find_package(MMRenderer PROVIDED_TARGETS MMRenderer::MMRenderer MODULE_NAME multimedia QMAKE_LIB mmrndclient)
qt_find_package(WrapPulseAudio PROVIDED_TARGETS WrapPulseAudio::WrapPulseAudio MODULE_NAME multimedia QMAKE_LIB pulseaudio)
qt_find_package(WMF PROVIDED_TARGETS WMF::WMF MODULE_NAME multimedia QMAKE_LIB wmf)
qt_find_package(EGL)

qt_find_package(FFmpeg OPTIONAL_COMPONENTS AVCODEC AVFORMAT AVUTIL AVDEVICE SWRESAMPLE SWSCALE PROVIDED_TARGETS FFmpeg::avcodec FFmpeg::avformat FFmpeg::avutil FFmpeg::avdevice FFmpeg::swresample  FFmpeg::swscale MODULE_NAME multimedia QMAKE_LIB ffmpeg)
qt_find_package(VAAPI COMPONENTS VA DRM PROVIDED_TARGETS VAAPI::VAAPI MODULE_NAME multimedia QMAKE_LIB vaapi)

#### Tests


qt_config_compile_test("evr"
                   LABEL "evr.h"
                   PROJECT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../config.tests/evr"
)

qt_config_compile_test("gpu_vivante"
                   LABEL "Vivante GPU"
                   PROJECT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../config.tests/gpu_vivante"
)

qt_config_compile_test("linux_v4l"
                   LABEL "Video for Linux"
                   PROJECT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../config.tests/linux_v4l"
)

qt_config_compile_test("wmsdk"
                   LABEL "wmsdk.h"
                   PROJECT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../config.tests/wmsdk"
)
qt_config_compile_test(linux_dmabuf
    LABEL "Linux DMA buffer support"
    LIBRARIES
        EGL::EGL
    CODE
"#include <EGL/egl.h>
#include <EGL/eglext.h>

int main(int, char **)
{
    /* BEGIN TEST: */
    eglCreateImage(nullptr,
        EGL_NO_CONTEXT,
        EGL_LINUX_DMA_BUF_EXT,
        nullptr,
        nullptr);
    /* END TEST: */
    return 0;
}
")

#### Features

qt_feature("ffmpeg" PRIVATE
    LABEL "FFmpeg"
    ENABLE INPUT_ffmpeg STREQUAL 'yes'
    DISABLE INPUT_ffmpeg STREQUAL 'no'
    CONDITION FFmpeg_FOUND AND (APPLE OR WIN32 OR ANDROID OR QNX OR QT_FEATURE_pulseaudio)
)
qt_feature("alsa" PUBLIC PRIVATE
    LABEL "ALSA (experimental)"
    AUTODETECT false
    CONDITION UNIX AND NOT QNX AND ALSA_FOUND AND NOT QT_FEATURE_pulseaudio
)
qt_feature("avfoundation" PUBLIC PRIVATE
    LABEL "AVFoundation"
    CONDITION AVFoundation_FOUND
)
qt_feature("coreaudio" PUBLIC PRIVATE
    LABEL "CoreAudio"
    CONDITION AVFoundation_FOUND
)
qt_feature("videotoolbox" PUBLIC PRIVATE
    LABEL "VideoToolbox"
    CONDITION AVFoundation_FOUND
)
qt_feature("evr" PUBLIC PRIVATE
    LABEL "evr.h"
    CONDITION WIN32 AND TEST_evr
)
qt_feature("gstreamer_1_0" PRIVATE
    LABEL "GStreamer 1.0"
    CONDITION GStreamer_FOUND
    ENABLE INPUT_gstreamer STREQUAL 'yes'
    DISABLE INPUT_gstreamer STREQUAL 'no'
)
qt_feature("gstreamer" PRIVATE
    CONDITION QT_FEATURE_gstreamer_1_0
)
qt_feature("gstreamer_app" PRIVATE
    LABEL "GStreamer App"
    CONDITION ( QT_FEATURE_gstreamer_1_0 AND GStreamer_App_FOUND )
)
qt_feature("gstreamer_photography" PRIVATE
    LABEL "GStreamer Photography"
    CONDITION ( QT_FEATURE_gstreamer_1_0 AND GStreamer_Photography_FOUND )
)
qt_feature("gstreamer_gl" PRIVATE
    LABEL "GStreamer OpenGL"
    CONDITION QT_FEATURE_opengl AND QT_FEATURE_gstreamer_1_0 AND GStreamer_Gl_FOUND
)

qt_feature("gpu_vivante" PRIVATE
    LABEL "Vivante GPU"
    CONDITION QT_FEATURE_gui AND QT_FEATURE_opengles2 AND TEST_gpu_vivante
)
qt_feature("linux_v4l" PRIVATE
    LABEL "Video for Linux"
    CONDITION UNIX AND TEST_linux_v4l
)
qt_feature("linux_dmabuf" PRIVATE
    LABEL "Linux DMA buffer support"
    CONDITION UNIX AND TEST_linux_dmabuf
)
qt_feature("vaapi" PRIVATE
    LABEL "VAAPI support"
    CONDITION UNIX AND VAAPI_FOUND AND QT_FEATURE_linux_dmabuf
)
qt_feature("mmrenderer" PUBLIC PRIVATE
    LABEL "MMRenderer"
    CONDITION MMRenderer_FOUND AND MMRendererCore_FOUND
    EMIT_IF QNX
)
qt_feature("pulseaudio" PUBLIC PRIVATE
    LABEL "PulseAudio"
    DISABLE INPUT_pulseaudio STREQUAL 'no'
    CONDITION WrapPulseAudio_FOUND
)
qt_feature("wmsdk" PRIVATE
    LABEL "Windows Media SDK"
    CONDITION WIN32 AND TEST_wmsdk
)
qt_feature("opensles" PRIVATE
    LABEL "Open SLES (Android)"
    CONDITION ANDROID
)
qt_feature("wasm" PRIVATE
    LABEL "Web Assembly"
    CONDITION WASM
)

qt_feature("wmf" PRIVATE
    LABEL "Windows Media Foundation"
    CONDITION WIN32 AND WMF_FOUND AND QT_FEATURE_wmsdk
)

qt_feature("spatialaudio" PRIVATE
    LABEL "Spatial Audio"
)
qt_feature("spatialaudio_quick3d" PRIVATE
    LABEL "Spatial Audio (Quick3D)"
    CONDITION TARGET Qt::Quick3D AND QT_FEATURE_spatialaudio
)

qt_configure_add_summary_section(NAME "Qt Multimedia")
qt_configure_add_summary_entry(ARGS "spatialaudio")
qt_configure_add_summary_entry(ARGS "spatialaudio_quick3d")
qt_configure_add_summary_section(NAME "Low level Audio Backend")
qt_configure_add_summary_entry(ARGS "alsa")
qt_configure_add_summary_entry(ARGS "pulseaudio")
qt_configure_add_summary_entry(ARGS "mmrenderer")
qt_configure_add_summary_entry(ARGS "coreaudio")
qt_configure_add_summary_entry(ARGS "wmsdk")
qt_configure_add_summary_entry(ARGS "opensles")
qt_configure_add_summary_entry(ARGS "wasm")
qt_configure_end_summary_section()
qt_configure_add_summary_section(NAME "Plugin")
qt_configure_add_summary_entry(ARGS "gstreamer_1_0")
qt_configure_add_summary_entry(ARGS "ffmpeg")
qt_configure_add_summary_entry(ARGS "mmrenderer")
qt_configure_add_summary_entry(ARGS "avfoundation")
qt_configure_add_summary_entry(ARGS "wmf")
qt_configure_end_summary_section()
qt_configure_add_summary_section(NAME "Hardware acceleration and features")
qt_configure_add_summary_entry(ARGS "linux_v4l")
qt_configure_add_summary_entry(ARGS "vaapi")
qt_configure_add_summary_entry(ARGS "linux_dmabuf")
qt_configure_add_summary_entry(ARGS "videotoolbox")
qt_configure_end_summary_section()
qt_configure_end_summary_section() # end of "Qt Multimedia" section

qt_configure_add_report_entry(
    TYPE WARNING
    MESSAGE "No backend for low level audio found."
    CONDITION NOT QT_FEATURE_alsa AND NOT QT_FEATURE_pulseaudio AND NOT QT_FEATURE_mmrenderer AND NOT QT_FEATURE_coreaudio AND NOT QT_FEATURE_wmsdk AND NOT ANDROID AND NOT WASM
)
