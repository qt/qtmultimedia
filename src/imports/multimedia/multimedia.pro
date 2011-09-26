TARGET = declarative_multimedia
TARGETPATH = Qt/multimediakit

include(../qimportbase.pri)

QT += declarative network multimediakit-private

DESTDIR = $$QT.multimediakit.imports/$$TARGETPATH
target.path = $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

HEADERS += \
        qdeclarativeaudio_p.h \
        qdeclarativemediabase_p.h \
        qdeclarativemediametadata_p.h \
        qdeclarativevideooutput_p.h \
        qsgvideonode_p.h \
        qsgvideonode_i420.h \
        qsgvideonode_rgb32.h \
        qdeclarativeradio_p.h \
        qdeclarativebackgroundaudio_p.h \
        qdeclarativecamera_p.h \
        qdeclarativecameracapture_p.h \
        qdeclarativecamerarecorder_p.h \
        qdeclarativecameraexposure_p.h \
        qdeclarativecameraflash_p.h \
        qdeclarativecamerafocus_p.h \
        qdeclarativecameraimageprocessing_p.h \
        qdeclarativecamerapreviewprovider_p.h

SOURCES += \
        multimedia.cpp \
        qdeclarativeaudio.cpp \
        qdeclarativemediabase.cpp \
        qdeclarativevideooutput.cpp \
        qsgvideonode.cpp \
        qsgvideonode_i420.cpp \
        qsgvideonode_rgb32.cpp \
        qdeclarativeradio.cpp \
        qdeclarativebackgroundaudio.cpp \
        qdeclarativecamera.cpp \
        qdeclarativecameracapture.cpp \
        qdeclarativecamerarecorder.cpp \
        qdeclarativecameraexposure.cpp \
        qdeclarativecameraflash.cpp \
        qdeclarativecamerafocus.cpp \
        qdeclarativecameraimageprocessing.cpp \
        qdeclarativecamerapreviewprovider.cpp

disabled {
    HEADERS += \
        qdeclarativevideo_p.h \


    SOURCES += \
        qdeclarativevideo.cpp \

}

qmldir.files += $$PWD/qmldir
qmldir.path +=  $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

INSTALLS += target qmldir
