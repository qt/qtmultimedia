TARGET = imx6vivantevideonode

QT += multimedia-private qtmultimediaquicktools-private

qtConfig(gstreamer_imxcommon) {
    QT += multimediagsttools-private
    QMAKE_USE += gstreamer_imxcommon
    DEFINES += GST_USE_UNSTABLE_API
}

HEADERS += \
    qsgvivantevideonode.h \
    qsgvivantevideomaterialshader.h \
    qsgvivantevideomaterial.h \
    qsgvivantevideonodefactory.h

SOURCES += \
    qsgvivantevideonode.cpp \
    qsgvivantevideomaterialshader.cpp \
    qsgvivantevideomaterial.cpp \
    qsgvivantevideonodefactory.cpp

OTHER_FILES += \
    imx6.json

PLUGIN_TYPE = video/videonode
PLUGIN_EXTENDS = quick
PLUGIN_CLASS_NAME = QSGVivanteVideoNodeFactory
load(qt_plugin)
