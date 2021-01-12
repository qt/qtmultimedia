QT += core-private

INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/androidmediaplayer_p.h \
    $$PWD/androidsurfacetexture_p.h \
    $$PWD/androidmediametadataretriever_p.h \
    $$PWD/androidcamera_p.h \
    $$PWD/androidmultimediautils_p.h \
    $$PWD/androidmediarecorder_p.h \
    $$PWD/androidsurfaceview_p.h

SOURCES += \
    $$PWD/androidmediaplayer.cpp \
    $$PWD/androidsurfacetexture.cpp \
    $$PWD/androidmediametadataretriever.cpp \
    $$PWD/androidcamera.cpp \
    $$PWD/androidmultimediautils.cpp \
    $$PWD/androidmediarecorder.cpp \
    $$PWD/androidsurfaceview.cpp
