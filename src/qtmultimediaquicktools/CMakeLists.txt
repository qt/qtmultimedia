# Generated from qtmultimediaquicktools.pro.

#####################################################################
## MultimediaQuick Module:
#####################################################################

qt_add_module(MultimediaQuick
    CONFIG_MODULE_NAME qtmultimediaquicktools
    INTERNAL_MODULE
    SOURCES
        qdeclarativevideooutput.cpp qdeclarativevideooutput_p.h
        qdeclarativevideooutput_backend_p.h
        qdeclarativevideooutput_render.cpp qdeclarativevideooutput_render_p.h
        qdeclarativevideooutput_window.cpp qdeclarativevideooutput_window_p.h
        qsgvideonode_p.cpp qsgvideonode_p.h
        qsgvideonode_rgb.cpp qsgvideonode_rgb_p.h
        qsgvideonode_texture.cpp qsgvideonode_texture_p.h
        qsgvideonode_yuv.cpp qsgvideonode_yuv_p.h
        qsgvideotexture.cpp qsgvideotexture_p.h
        qtmultimediaquickdefs_p.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::MultimediaPrivate
        Qt::Quick
        Qt::QuickPrivate
)

# Resources:
set(qtmultimediaquicktools_resource_files
    "shaders/bgra.frag.qsb"
    "shaders/nv12.frag.qsb"
    "shaders/nv21.frag.qsb"
    "shaders/rgba.frag.qsb"
    "shaders/rgba.vert.qsb"
    "shaders/uyvy.frag.qsb"
    "shaders/yuv.vert.qsb"
    "shaders/yuv_yv.frag.qsb"
    "shaders/yuyv.frag.qsb"
)

qt_add_resource(MultimediaQuick "qtmultimediaquicktools"
    PREFIX
        "/qtmultimediaquicktools"
    FILES
        ${qtmultimediaquicktools_resource_files}
)

