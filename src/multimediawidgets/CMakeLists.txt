# Generated from multimediawidgets.pro.

#####################################################################
## MultimediaWidgets Module:
#####################################################################

qt_add_module(MultimediaWidgets
    SOURCES
        qcameraviewfinder.cpp qcameraviewfinder.h
        qpaintervideosurface.cpp qpaintervideosurface_p.h
        qtmultimediawidgetdefs.h
        qvideowidget.cpp qvideowidget.h qvideowidget_p.h
        qvideowidgetcontrol.cpp qvideowidgetcontrol.h
    LIBRARIES
        Qt::MultimediaPrivate
        Qt::WidgetsPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Multimedia
        Qt::Widgets
    PRIVATE_MODULE_INTERFACE
        Qt::WidgetsPrivate
)

## Scopes:
#####################################################################

qt_extend_target(MultimediaWidgets CONDITION TARGET Qt::OpenGL
    LIBRARIES
        Qt::OpenGL
    PUBLIC_LIBRARIES
        Qt::OpenGLWidgets
)

qt_extend_target(MultimediaWidgets CONDITION QT_FEATURE_graphicsview
    SOURCES
        qgraphicsvideoitem.cpp qgraphicsvideoitem.h # special case
)

qt_extend_target(MultimediaWidgets CONDITION QT_FEATURE_gstreamer
    SOURCES
        gstreamer/qgstreamervideowidget.cpp gstreamer/qgstreamervideowidget_p.h
)

#### Keys ignored in scope 3:.:.:multimediawidgets.pro:QT_FEATURE_graphicsview:
# PUBLIC_HEADERS = "qgraphicsvideoitem.h"

qt_extend_target(MultimediaWidgets CONDITION MSVC AND (QMAKE_MSC_VER LESS 1900)
    COMPILE_OPTIONS
        -Zm200
)
