TARGET = QtMultimediaQuick_p
QT = core quick multimedia-private
CONFIG += internal_module

load(qt_module)

DEFINES += QT_BUILD_QTMM_QUICK_LIB

# Header files must go inside source directory of a module
# to be installed by syncqt.
INCLUDEPATH += ../multimedia/qtmultimediaquicktools_headers/

PRIVATE_HEADERS += \
    ../multimedia/qtmultimediaquicktools_headers/qdeclarativevideooutput_p.h \
    ../multimedia/qtmultimediaquicktools_headers/qdeclarativevideooutput_backend_p.h \
    ../multimedia/qtmultimediaquicktools_headers/qsgvideonode_p.h \
    ../multimedia/qtmultimediaquicktools_headers/qtmultimediaquickdefs_p.h

SOURCES += \
    qsgvideonode_p.cpp \
    qdeclarativevideooutput.cpp \
    qdeclarativevideooutput_render.cpp \
    qdeclarativevideooutput_window.cpp \
    qsgvideonode_yuv.cpp \
    qsgvideonode_rgb.cpp \
    qsgvideonode_texture.cpp

HEADERS += \
    $$PRIVATE_HEADERS \
    qdeclarativevideooutput_render_p.h \
    qdeclarativevideooutput_window_p.h \
    qsgvideonode_yuv.h \
    qsgvideonode_rgb.h \
    qsgvideonode_texture.h

RESOURCES += \
    qtmultimediaquicktools.qrc

OTHER_FILES += \
    shaders/rgbvideo.vert \
    shaders/rgbvideo_padded.vert \
    shaders/rgbvideo.frag \
    shaders/rgbvideo_swizzle.frag \
    shaders/biplanaryuvvideo.vert \
    shaders/biplanaryuvvideo.frag \
    shaders/biplanaryuvvideo_swizzle.frag \
    shaders/triplanaryuvvideo.vert \
    shaders/triplanaryuvvideo.frag
