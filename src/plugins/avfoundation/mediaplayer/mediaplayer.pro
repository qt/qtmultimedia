load(qt_build_config)

#DEFINES += QT_DEBUG_AVF
# Avoid clash with a variable named `slots' in a Quartz header
CONFIG += no_keywords

TARGET = qavfmediaplayer
QT += multimedia-private network

PLUGIN_TYPE = mediaservice
PLUGIN_CLASS_NAME = AVFMediaPlayerServicePlugin
load(qt_plugin)

LIBS += -framework AVFoundation -framework CoreMedia

DEFINES += QMEDIA_AVF_MEDIAPLAYER

HEADERS += \
    avfmediaplayercontrol.h \
    avfmediaplayermetadatacontrol.h \
    avfmediaplayerservice.h \
    avfmediaplayersession.h \
    avfmediaplayerserviceplugin.h \
    avfvideorenderercontrol.h \
    avfdisplaylink.h \
    avfvideoframerenderer.h \
    avfvideooutput.h

OBJECTIVE_SOURCES += \
    avfmediaplayercontrol.mm \
    avfmediaplayermetadatacontrol.mm \
    avfmediaplayerservice.mm \
    avfmediaplayerserviceplugin.mm \
    avfmediaplayersession.mm \
    avfvideorenderercontrol.mm \
    avfdisplaylink.mm \
    avfvideoframerenderer.mm \
    avfvideooutput.mm

qtHaveModule(widgets) {
    QT += multimediawidgets-private opengl
    HEADERS += \
        avfvideowidgetcontrol.h \
        avfvideowidget.h

    OBJECTIVE_SOURCES += \
        avfvideowidgetcontrol.mm \
        avfvideowidget.mm
}

OTHER_FILES += \
    avfmediaplayer.json
