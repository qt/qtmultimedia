load(qt_module)

TARGET = QtMultimedia
QPRO_PWD = $$PWD
QT = core network gui

CONFIG += module
MODULE_PRI += ../../modules/qt_multimedia.pri

contains(QT_CONFIG, opengl) | contains(QT_CONFIG, opengles2) {
} else {
   DEFINES += QT_NO_OPENGL
}

!static:DEFINES += QT_MAKEDLL
DEFINES += QT_BUILD_MULTIMEDIA_LIB

load(qt_module_config)

HEADERS += qtmultimediaversion.h


PRIVATE_HEADERS += \
    qmediacontrol_p.h \
    qmediaobject_p.h \
    qmediaservice_p.h  \
    qmediaplaylist_p.h \
    qmediaplaylistprovider_p.h \
    qmediaimageviewerservice_p.h \
    qmediapluginloader_p.h \
    qvideosurfaceoutput_p.h

PUBLIC_HEADERS += \
    qmediacontrol.h \
    qmediaobject.h \
    qmediaservice.h \
    qmediabindableinterface.h \
    qlocalmediaplaylistprovider.h \
    qmediaimageviewer.h \
    qmediaplayer.h \
    qmediaplayercontrol.h \
    qmediaplaylist.h \
    qmediaplaylistnavigator.h \
    qmediaplaylistprovider.h \
    qmediaplaylistioplugin.h \
    qmediabackgroundplaybackcontrol.h \
    qmediacontent.h \
    qmediaresource.h \
    qmediarecorder.h \
    qmediaencodersettings.h \
    qmediarecordercontrol.h \
    qmediaserviceprovider.h \
    qmediaserviceproviderplugin.h \
    qmetadatareadercontrol.h \
    qmetadatawritercontrol.h \
    qmediastreamscontrol.h \
    qradiotuner.h \
    qradiodata.h \
    qradiotunercontrol.h \
    qradiodatacontrol.h \
    qtmedianamespace.h \
    qaudioencodercontrol.h \
    qvideoencodercontrol.h \
    qimageencodercontrol.h \
    qaudiocapturesource.h \
    qmediacontainercontrol.h \
    qmediaplaylistcontrol.h \
    qmediaplaylistsourcecontrol.h \
    qaudioendpointselector.h \
    qvideodevicecontrol.h \
    qvideorenderercontrol.h \
    qmediatimerange.h \
    qmedianetworkaccesscontrol.h \
    qmediaenumdebug.h \
    qtmultimediadefs.h

SOURCES += qmediacontrol.cpp \
    qmediaobject.cpp \
    qmediaservice.cpp \
    qmediabindableinterface.cpp \
    qlocalmediaplaylistprovider.cpp \
    qmediaimageviewer.cpp \
    qmediaimageviewerservice.cpp \
    qmediaplayer.cpp \
    qmediaplayercontrol.cpp \
    qmediaplaylist.cpp \
    qmediaplaylistioplugin.cpp \
    qmediaplaylistnavigator.cpp \
    qmediaplaylistprovider.cpp \
    qmediarecorder.cpp \
    qmediaencodersettings.cpp \
    qmediarecordercontrol.cpp \
    qmediacontent.cpp \
    qmediaresource.cpp \
    qmediaserviceprovider.cpp \
    qmetadatareadercontrol.cpp \
    qmetadatawritercontrol.cpp \
    qmediastreamscontrol.cpp \
    qradiotuner.cpp \
    qradiodata.cpp \
    qradiotunercontrol.cpp \
    qradiodatacontrol.cpp \
    qaudioencodercontrol.cpp \
    qvideoencodercontrol.cpp \
    qimageencodercontrol.cpp \
    qaudiocapturesource.cpp \
    qmediacontainercontrol.cpp \
    qmediaplaylistcontrol.cpp \
    qmediaplaylistsourcecontrol.cpp \
    qaudioendpointselector.cpp \
    qvideodevicecontrol.cpp \
    qmediapluginloader.cpp \
    qvideorenderercontrol.cpp \
    qmediatimerange.cpp \
    qmedianetworkaccesscontrol.cpp \
    qvideosurfaceoutput.cpp \
    qmediabackgroundplaybackcontrol.cpp \
    qtmedianamespace.cpp

#Camera
PUBLIC_HEADERS += \
    qcamera.h \
    qcameraimagecapture.h \
    qcameraimagecapturecontrol.h \
    qcameraexposure.h \
    qcamerafocus.h \
    qcameraimageprocessing.h \
    qcameracontrol.h \
    qcameralockscontrol.h \
    qcameraexposurecontrol.h \
    qcamerafocuscontrol.h \
    qcameraflashcontrol.h \
    qcameraimageprocessingcontrol.h \
    qcameracapturedestinationcontrol.h \
    qcameracapturebufferformatcontrol.h

SOURCES += \
    qcamera.cpp \
    qcameraexposure.cpp \
    qcamerafocus.cpp \
    qcameraimageprocessing.cpp \
    qcameraimagecapture.cpp \
    qcameraimagecapturecontrol.cpp \
    qcameracontrol.cpp \
    qcameralockscontrol.cpp \
    qcameraexposurecontrol.cpp \
    qcamerafocuscontrol.cpp \
    qcameraflashcontrol.cpp \
    qcameraimageprocessingcontrol.cpp \
    qcameracapturedestinationcontrol.cpp \
    qcameracapturebufferformatcontrol.cpp

include(audio/audio.pri)
include(video/video.pri)
include(effects/effects.pri)

mac {
   LIBS += -framework AppKit -framework QuartzCore -framework QTKit
}

HEADERS += $$PUBLIC_HEADERS $$PRIVATE_HEADERS
