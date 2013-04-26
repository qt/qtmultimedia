QT += qml quick network multimedia-private qtmultimediaquicktools-private

HEADERS += \
        qdeclarativeaudio_p.h \
        qdeclarativemediametadata_p.h \
        qdeclarativevideooutput_p.h \
        qdeclarativevideooutput_backend_p.h \
        qdeclarativevideooutput_render_p.h \
        qdeclarativevideooutput_window_p.h \
        qsgvideonode_i420.h \
        qsgvideonode_rgb.h \
        qsgvideonode_texture.h \
        qdeclarativeradio_p.h \
        qdeclarativeradiodata_p.h \
        qdeclarativecamera_p.h \
        qdeclarativecameracapture_p.h \
        qdeclarativecamerarecorder_p.h \
        qdeclarativecameraexposure_p.h \
        qdeclarativecameraflash_p.h \
        qdeclarativecamerafocus_p.h \
        qdeclarativecameraimageprocessing_p.h \
        qdeclarativecamerapreviewprovider_p.h \
        qdeclarativetorch_p.h

SOURCES += \
        multimedia.cpp \
        qdeclarativeaudio.cpp \
        qdeclarativevideooutput.cpp \
        qdeclarativevideooutput_render.cpp \
        qdeclarativevideooutput_window.cpp \
        qsgvideonode_i420.cpp \
        qsgvideonode_rgb.cpp \
        qsgvideonode_texture.cpp \
        qdeclarativeradio.cpp \
        qdeclarativeradiodata.cpp \
        qdeclarativecamera.cpp \
        qdeclarativecameracapture.cpp \
        qdeclarativecamerarecorder.cpp \
        qdeclarativecameraexposure.cpp \
        qdeclarativecameraflash.cpp \
        qdeclarativecamerafocus.cpp \
        qdeclarativecameraimageprocessing.cpp \
        qdeclarativecamerapreviewprovider.cpp \
        qdeclarativetorch.cpp

QML_FILES += \
    Video.qml

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

load(qml_plugin)
