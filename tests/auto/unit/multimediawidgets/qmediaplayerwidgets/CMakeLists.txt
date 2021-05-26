# Generated from qmediaplayerwidgets.pro.

#####################################################################
## tst_qmediaplayerwidgets Test:
#####################################################################

qt_internal_add_test(tst_qmediaplayerwidgets
    SOURCES
        tst_qmediaplayerwidgets.cpp
    INCLUDE_DIRECTORIES
        ../../mockbackend
    PUBLIC_LIBRARIES
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MultimediaWidgetsPrivate
        Qt::Network
        Qt::Widgets
        QtMultimediaMockBackend
)
