# Generated from qvideowidget.pro.

#####################################################################
## tst_qvideowidget Test:
#####################################################################

qt_internal_add_test(tst_qvideowidget
    SOURCES
        tst_qvideowidget.cpp
    INCLUDE_DIRECTORIES
        ../../mockbackend
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MultimediaWidgetsPrivate
        Qt::Widgets
        QtMultimediaMockBackend
)
