# Generated from imx6.pro.

#####################################################################
## QSGVivanteVideoNodeFactory Plugin:
#####################################################################

qt_internal_add_plugin(QSGVivanteVideoNodeFactory
    OUTPUT_NAME imx6vivantevideonode
    TYPE video/videonode
    SOURCES
        qsgvivantevideomaterial.cpp qsgvivantevideomaterial.h
        qsgvivantevideomaterialshader.cpp qsgvivantevideomaterialshader.h
        qsgvivantevideonode.cpp qsgvivantevideonode.h
        qsgvivantevideonodefactory.cpp qsgvivantevideonodefactory.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MultimediaQuickPrivate
        gstreamer
)

# Resources:
set(imx6_resource_files
    "shaders/rgba.frag.qsb"
    "shaders/rgba.vert.qsb"
)

qt_internal_add_resource(QSGVivanteVideoNodeFactory "imx6"
    PREFIX
        "/imx6"
    FILES
        ${imx6_resource_files}
)


#### Keys ignored in scope 1:.:.:imx6.pro:<TRUE>:
# OTHER_FILES = "imx6.json"
# PLUGIN_EXTENDS = "quick"
