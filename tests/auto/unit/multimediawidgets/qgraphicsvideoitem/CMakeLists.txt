# Generated from qgraphicsvideoitem.pro.

#####################################################################
## tst_qgraphicsvideoitem Test:
#####################################################################

qt_internal_add_test(tst_qgraphicsvideoitem
    SOURCES
        tst_qgraphicsvideoitem.cpp
    INCLUDE_DIRECTORIES
        ../../mockbackend
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MultimediaWidgetsPrivate
        Qt::Widgets
        QtMultimediaMockBackend
)
