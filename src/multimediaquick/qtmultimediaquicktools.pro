TARGET = QtMultimediaQuick

QT = core quick multimedia-private quick-private
CONFIG += internal_module

PRIVATE_HEADERS += \
    qdeclarativevideooutput_p.h \
    qsgvideonode_p.h \
    qsgvideotexture_p.h \
    qtmultimediaquickdefs_p.h

HEADERS += \
    $$PRIVATE_HEADERS \

SOURCES += \
    qsgvideonode_p.cpp \
    qsgvideotexture.cpp \
    qdeclarativevideooutput.cpp \

SOURCES += qdeclarativevideooutput_render.cpp \
           qsgvideonode_rgb.cpp \
           qsgvideonode_yuv.cpp \
           qsgvideonode_texture.cpp
HEADERS += qdeclarativevideooutput_render_p.h \
           qsgvideonode_rgb_p.h \
           qsgvideonode_yuv_p.h \
           qsgvideonode_texture_p.h

RESOURCES += \
    qtmultimediaquicktools.qrc

load(qt_module)
