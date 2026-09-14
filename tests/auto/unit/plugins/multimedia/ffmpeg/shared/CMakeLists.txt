include_guard(GLOBAL)

add_library(ffmpeg_test_support INTERFACE
    nv12hwframecontext_p.h)
target_include_directories(ffmpeg_test_support INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(ffmpeg_test_support INTERFACE
    Qt::FFmpegMediaPluginImplPrivate
    Qt::Core
)
