# Generated from qtmultimediaquicktools.pro.

#####################################################################
## MultimediaQuick Module:
#####################################################################

qt_add_module(MultimediaQuick
    CONFIG_MODULE_NAME qtmultimediaquicktools
    INTERNAL_MODULE
    SOURCES
        qdeclarativevideooutput.cpp qdeclarativevideooutput_p.h
        qdeclarativevideooutput_render.cpp qdeclarativevideooutput_render_p.h
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
