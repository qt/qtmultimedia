# Generated from qpaintervideosurface.pro.

#####################################################################
## tst_qpaintervideosurface Test:
#####################################################################

qt_add_test(tst_qpaintervideosurface
    SOURCES
        tst_qpaintervideosurface.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MultimediaWidgetsPrivate
        Qt::Widgets
)

## Scopes:
#####################################################################

qt_extend_target(tst_qpaintervideosurface CONDITION TARGET Qt::OpenGL
    PUBLIC_LIBRARIES
        Qt::OpenGL
)
