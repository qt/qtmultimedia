# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from imx6.pro.

#####################################################################
## QSGVivanteVideoNodeFactory Plugin:
#####################################################################

qt_internal_add_plugin(QSGVivanteVideoNodeFactoryPlugin
    OUTPUT_NAME imx6vivantevideonode
    PLUGIN_TYPE video/videonode
    CLASS_NAME QSGVivanteVideoNodeFactory
    SOURCES
        qsgvivantevideomaterial.cpp qsgvivantevideomaterial.h
        qsgvivantevideomaterialshader.cpp qsgvivantevideomaterialshader.h
        qsgvivantevideonode.cpp qsgvivantevideonode.h
        qsgvivantevideonodefactory.cpp qsgvivantevideonodefactory.h
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MultimediaQuickPrivate
        GStreamer::GStreamer # special case
)

# Resources:
set(imx6_resource_files
    "shaders/rgba.frag.qsb"
    "shaders/rgba.vert.qsb"
)

qt_internal_add_resource(QSGVivanteVideoNodeFactoryPlugin "imx6"
    PREFIX
        "/imx6"
    FILES
        ${imx6_resource_files}
)


#### Keys ignored in scope 1:.:.:imx6.pro:<TRUE>:
# OTHER_FILES = "imx6.json"
# PLUGIN_EXTENDS = "quick"
