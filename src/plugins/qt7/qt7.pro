# Avoid clash with a variable named `slots' in a Quartz header
CONFIG += no_keywords

TARGET = qqt7engine
QT += multimedia-private network
qtHaveModule(widgets) {
    QT += multimediawidgets-private widgets
}

PLUGIN_TYPE = mediaservice
PLUGIN_CLASS_NAME = QT7ServicePlugin
load(qt_plugin)

!simulator {
QT += opengl
}

#DEFINES += QT_DEBUG_QT7

LIBS += -framework AppKit -framework AudioUnit \
        -framework AudioToolbox -framework CoreAudio \
        -framework QuartzCore -framework QTKit

# QUICKTIME_C_API_AVAILABLE is true only on i386
# so make sure to link QuickTime
contains(QT_ARCH, i386) {
    LIBS += -framework QuickTime
}

HEADERS += \
    qt7backend.h \
    qt7videooutput.h \
    qt7serviceplugin.h

OBJECTIVE_SOURCES += \
    qt7backend.mm \
    qt7serviceplugin.mm

!simulator {
    HEADERS += \
        qt7movieviewoutput.h \
        qt7movierenderer.h \
        qt7ciimagevideobuffer.h \
        qcvdisplaylink.h

    OBJECTIVE_SOURCES += \
        qt7movieviewoutput.mm \
        qt7movierenderer.mm \
        qt7videooutput.mm \
        qt7ciimagevideobuffer.mm \
        qcvdisplaylink.mm

    qtHaveModule(widgets) {
        HEADERS += \
            qt7movieviewrenderer.h \
            qt7movievideowidget.h

        OBJECTIVE_SOURCES += \
            qt7movieviewrenderer.mm \
            qt7movievideowidget.mm
    }
}

include(mediaplayer/mediaplayer.pri)

OTHER_FILES += \
    qt7.json
