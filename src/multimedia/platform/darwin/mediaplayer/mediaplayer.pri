QT += opengl network

HEADERS += \
    $$PWD/avfmediaplayercontrol_p.h \
    $$PWD/avfmediaplayermetadatacontrol_p.h \
    $$PWD/avfmediaplayerservice_p.h \
    $$PWD/avfmediaplayersession_p.h \
    $$PWD/avfmediaplayerserviceplugin_p.h \
    $$PWD/avfvideooutput_p.h \
    $$PWD/avfvideowindowcontrol_p.h

SOURCES += \
    $$PWD/avfmediaplayercontrol.mm \
    $$PWD/avfmediaplayermetadatacontrol.mm \
    $$PWD/avfmediaplayerservice.mm \
    $$PWD/avfmediaplayerserviceplugin.mm \
    $$PWD/avfmediaplayersession.mm \
    $$PWD/avfvideooutput.mm \
    $$PWD/avfvideowindowcontrol.mm

ios|tvos {
    qtConfig(opengl) {
        HEADERS += \
            $$PWD/avfvideoframerenderer_ios_p.h \
            $$PWD/avfvideorenderercontrol_p.h \
            $$PWD/avfdisplaylink_p.h

        SOURCES += \
            $$PWD/avfvideoframerenderer_ios.mm \
            $$PWD/avfvideorenderercontrol.mm \
            $$PWD/avfdisplaylink.mm
    }
    LIBS += -framework Foundation
} else {
    LIBS += -framework AppKit

    qtConfig(opengl) {
        HEADERS += \
            $$PWD/avfvideoframerenderer_p.h \
            $$PWD/avfvideorenderercontrol_p.h \
            $$PWD/avfdisplaylink_p.h

        SOURCES += \
            $$PWD/avfvideoframerenderer.mm \
            $$PWD/avfvideorenderercontrol.mm \
            $$PWD/avfdisplaylink.mm
    }
}
