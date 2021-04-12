
INCLUDEPATH += video

PUBLIC_HEADERS += \
    video/qabstractvideosurface.h \
    video/qvideoframe.h \
    video/qvideoframeformat.h \

PRIVATE_HEADERS += \
    video/qabstractvideobuffer_p.h \
    video/qimagevideobuffer_p.h \
    video/qmemoryvideobuffer_p.h \
    video/qvideooutputorientationhandler_p.h \
    video/qvideoframeconversionhelper_p.h \
    video/qvideosurfaces_p.h

SOURCES += \
    video/qabstractvideobuffer.cpp \
    video/qabstractvideosurface.cpp \
    video/qimagevideobuffer.cpp \
    video/qmemoryvideobuffer.cpp \
    video/qvideoframe.cpp \
    video/qvideooutputorientationhandler.cpp \
    video/qvideoframeformat.cpp \
    video/qvideoframeconversionhelper.cpp \
    video/qvideosurfaces.cpp

SSE2_SOURCES += video/qvideoframeconversionhelper_sse2.cpp
SSSE3_SOURCES += video/qvideoframeconversionhelper_ssse3.cpp
AVX2_SOURCES += video/qvideoframeconversionhelper_avx2.cpp
