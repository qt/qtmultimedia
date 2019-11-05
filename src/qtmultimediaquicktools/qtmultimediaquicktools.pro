TARGET = QtMultimediaQuick

QT = core quick multimedia-private
CONFIG += internal_module

PRIVATE_HEADERS += \
    qdeclarativevideooutput_p.h \
    qdeclarativevideooutput_backend_p.h \
    qsgvideonode_p.h \
    qtmultimediaquickdefs_p.h

HEADERS += \
    $$PRIVATE_HEADERS \
    qdeclarativevideooutput_window_p.h

SOURCES += \
    qsgvideonode_p.cpp \
    qdeclarativevideooutput.cpp \
    qdeclarativevideooutput_window.cpp

qtConfig(opengl) {
    SOURCES += qdeclarativevideooutput_render.cpp \
               qsgvideonode_rgb.cpp \
               qsgvideonode_yuv.cpp \
               qsgvideonode_texture.cpp
    HEADERS += qdeclarativevideooutput_render_p.h \
               qsgvideonode_rgb_p.h \
               qsgvideonode_yuv_p.h \
               qsgvideonode_texture_p.h
}

RESOURCES += \
    qtmultimediaquicktools.qrc

OTHER_FILES += \
    shaders/monoplanarvideo.vert \
    shaders/rgbvideo_padded.vert \
    shaders/rgbvideo.frag \
    shaders/rgbvideo_swizzle.frag \
    shaders/biplanaryuvvideo.vert \
    shaders/biplanaryuvvideo.frag \
    shaders/biplanaryuvvideo_swizzle.frag \
    shaders/triplanaryuvvideo.vert \
    shaders/triplanaryuvvideo.frag \
    shaders/uyvyvideo.frag \
    shaders/yuyvvideo.frag

load(qt_module)
