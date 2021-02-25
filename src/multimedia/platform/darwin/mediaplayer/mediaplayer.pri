QT += opengl network

HEADERS += \
    $$PWD/avfmediaplayer_p.h \
    $$PWD/avfmetadata_p.h \
    $$PWD/avfvideooutput_p.h \
    $$PWD/avfvideowindowcontrol_p.h

SOURCES += \
    $$PWD/avfmediaplayer.mm \
    $$PWD/avfmetadata.mm \
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
