load(qt_module)

# distinct from QtMultimedia
TARGET = QtMultimediaKit
QPRO_PWD = $$PWD
QT = core network gui

CONFIG += module
MODULE_PRI += ../../modules/qt_multimediakit.pri

contains(QT_CONFIG, opengl) | contains(QT_CONFIG, opengles2): !symbian {
   QT += opengl
} else {
   DEFINES += QT_NO_OPENGL
}

!static:DEFINES += QT_MAKEDLL
DEFINES += QT_BUILD_MULTIMEDIA_LIB

load(qt_module_config)

HEADERS += qtmultimediakitversion.h

PRIVATE_HEADERS += \
    qmediacontrol_p.h \
    qmediaobject_p.h \
    qmediaservice_p.h  \
    qmediaplaylist_p.h \
    qmediaplaylistprovider_p.h \
    qmediaimageviewerservice_p.h \
    qvideowidget_p.h \
    qmediapluginloader_p.h \
    qpaintervideosurface_p.h \
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
    qradiotunercontrol.h \
    qtmedianamespace.h \
    qvideowidget.h \
    qvideowindowcontrol.h \
    qvideowidgetcontrol.h \
    qaudioencodercontrol.h \
    qvideoencodercontrol.h \
    qimageencodercontrol.h \
    qaudiocapturesource.h \
    qmediacontainercontrol.h \
    qmediaplaylistcontrol.h \
    qmediaplaylistsourcecontrol.h \
    qaudioendpointselector.h \
    qvideodevicecontrol.h \
    qgraphicsvideoitem.h \
    qvideorenderercontrol.h \
    qmediatimerange.h \
    qmedianetworkaccesscontrol.h \
    qmediaenumdebug.h

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
    qradiotunercontrol.cpp \
    qvideowidget.cpp \
    qvideowindowcontrol.cpp \
    qvideowidgetcontrol.cpp \
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
    qpaintervideosurface.cpp \
    qvideorenderercontrol.cpp \
    qmediatimerange.cpp \
    qmedianetworkaccesscontrol.cpp \
    qvideosurfaceoutput.cpp

#Camera
PUBLIC_HEADERS += \
    qcamera.h \
    qcameraviewfinder.h \
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
    qcameraviewfinder.cpp \
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
!simulator {
   HEADERS += qpaintervideosurface_mac_p.h
   OBJECTIVE_SOURCES += qpaintervideosurface_mac.mm
}
   LIBS += -framework AppKit -framework QuartzCore -framework QTKit
}

maemo5 {
    isEqual(QT_ARCH,armv6):QMAKE_CXXFLAGS += -march=armv7a -mcpu=cortex-a8 -mfloat-abi=softfp -mfpu=neon
    HEADERS += qxvideosurface_maemo5_p.h
    SOURCES += qxvideosurface_maemo5.cpp
    SOURCES += qgraphicsvideoitem_maemo5.cpp
    LIBS += -lXv  -lX11 -lXext
}

maemo6 {
    isEqual(QT_ARCH,armv6) {
        HEADERS += qeglimagetexturesurface_p.h
        SOURCES += qeglimagetexturesurface.cpp

        SOURCES += qgraphicsvideoitem_maemo6.cpp

        LIBS += -lX11
    } else {
        SOURCES += qgraphicsvideoitem.cpp
    }
}

symbian {
    contains(surfaces_s60_enabled, yes) {
        SOURCES += qgraphicsvideoitem_symbian.cpp
    } else {
        SOURCES += qgraphicsvideoitem_overlay.cpp
    }
}

!maemo*:!symbian {
    SOURCES += qgraphicsvideoitem.cpp
}

HEADERS += $$PUBLIC_HEADERS $$PRIVATE_HEADERS

symbian {
    contains(S60_VERSION, 5.1) |contains (S60_VERSION, 3.2) | contains(S60_VERSION, 3.1): DEFINES += PRE_S60_52_PLATFORM
    load(data_caging_paths)
    QtMediaDeployment.sources = QtMultimediaKit.dll
    QtMediaDeployment.path = /sys/bin
    DEPLOYMENT += QtMediaDeployment
    TARGET.UID3=0x2002AC77
    TARGET.CAPABILITY = ALL -TCB
    LIBS += -lefsrv
}

# CONFIG += middleware
# include(../../features/deploy.pri)
