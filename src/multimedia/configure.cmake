

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
if((QNX) OR QT_FIND_ALL_PACKAGES_ALWAYS)
    qt_find_package(MMRenderer PROVIDED_TARGETS MMRenderer::MMRenderer MODULE_NAME multimedia QMAKE_LIB mmrenderer)
endif()
qt_find_package(WrapPulseAudio PROVIDED_TARGETS WrapPulseAudio::WrapPulseAudio MODULE_NAME multimedia QMAKE_LIB pulseaudio)
qt_find_package(WMF PROVIDED_TARGETS WMF::WMF MODULE_NAME multimedia QMAKE_LIB wmf)
qt_find_package(EGL)


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

qt_feature("alsa" PUBLIC PRIVATE
    LABEL "ALSA"
    AUTODETECT false
    CONDITION UNIX AND NOT QNX AND ALSA_FOUND AND NOT QT_FEATURE_gstreamer AND NOT QT_FEATURE_pulseaudio
)
qt_feature_definition("alsa" "QT_NO_ALSA" NEGATE VALUE "1")
qt_feature("avfoundation" PUBLIC PRIVATE
    LABEL "AVFoundation"
    CONDITION AVFoundation_FOUND
)
qt_feature_definition("avfoundation" "QT_NO_AVFOUNDATION" NEGATE VALUE "1")
qt_feature("evr" PUBLIC PRIVATE
    LABEL "evr.h"
    CONDITION WIN32 AND TEST_evr
)
qt_feature_definition("evr" "QT_NO_EVR" NEGATE VALUE "1")
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
qt_feature("mmrenderer" PUBLIC PRIVATE
    LABEL "MMRenderer"
    CONDITION MMRenderer_FOUND AND false
    EMIT_IF QNX
)
qt_feature_definition("mmrenderer" "QT_NO_MMRENDERER" NEGATE VALUE "1")
qt_feature("pulseaudio" PUBLIC PRIVATE
    LABEL "PulseAudio"
    AUTODETECT false
    CONDITION WrapPulseAudio_FOUND AND NOT QT_FEATURE_gstreamer
)
qt_feature_definition("pulseaudio" "QT_NO_PULSEAUDIO" NEGATE VALUE "1")
qt_feature("wmsdk" PRIVATE
    LABEL "wmsdk.h"
    CONDITION WIN32 AND TEST_wmsdk
)
qt_feature("wmf" PRIVATE
    LABEL "Windows Media Foundation"
    CONDITION WIN32 AND WMF_FOUND AND QT_FEATURE_wmsdk
)
qt_configure_add_summary_section(NAME "Qt Multimedia")
#qt_configure_add_summary_entry(ARGS "alsa")
qt_configure_add_summary_entry(ARGS "gstreamer_1_0")
qt_configure_add_summary_entry(ARGS "linux_v4l")
#qt_configure_add_summary_entry(ARGS "pulseaudio")
qt_configure_add_summary_entry(ARGS "linux_dmabuf")
qt_configure_add_summary_entry(ARGS "mmrenderer")
qt_configure_add_summary_entry(ARGS "avfoundation")
qt_configure_add_summary_entry(ARGS "wmf")
qt_configure_end_summary_section() # end of "Qt Multimedia" section
