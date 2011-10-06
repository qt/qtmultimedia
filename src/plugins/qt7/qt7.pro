load(qt_module)

TARGET = qqt7engine
QT += multimedia-private multimediawidgets-private network
PLUGIN_TYPE = mediaservice

load(qt_plugin)
DESTDIR = $$QT.multimedia.plugins/$${PLUGIN_TYPE}

!simulator {
QT += opengl
}

#DEFINES += QT_DEBUG_QT7

LIBS += -framework AppKit -framework AudioUnit \
        -framework AudioToolbox -framework CoreAudio \
        -framework QuartzCore -framework QTKit

# The Quicktime framework is only awailable for 32-bit builds, so we
# need to check for this before linking against it.
# QMAKE_MAC_XARCH is not awailable on Tiger, but at the same time,
# we never build for 64-bit architechtures on Tiger either:
contains(QMAKE_MAC_XARCH, no) {
    LIBS += -framework QuickTime
} else {
    LIBS += -Xarch_i386 -framework QuickTime -Xarch_ppc -framework QuickTime
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
        qt7movievideowidget.h \
        qt7movieviewrenderer.h \
        qt7movierenderer.h \
        qt7ciimagevideobuffer.h \
        qcvdisplaylink.h

    OBJECTIVE_SOURCES += \
        qt7movieviewoutput.mm \
        qt7movievideowidget.mm \
        qt7movieviewrenderer.mm \
        qt7movierenderer.mm \
        qt7videooutput.mm \
        qt7ciimagevideobuffer.mm \
        qcvdisplaylink.mm
}

include(mediaplayer/mediaplayer.pri)

target.path += $$[QT_INSTALL_PLUGINS]/$${PLUGIN_TYPE}
INSTALLS += target
