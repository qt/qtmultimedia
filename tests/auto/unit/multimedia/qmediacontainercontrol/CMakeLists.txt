# Generated from qmediacontainercontrol.pro.

#####################################################################
## tst_qmediacontainercontrol Test:
#####################################################################

qt_add_test(tst_qmediacontainercontrol
    SOURCES
        tst_qmediacontainercontrol.cpp
    INCLUDE_DIRECTORIES
        ../../mockbackend
    PUBLIC_LIBRARIES
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}
        Qt::Gui
        Qt::MultimediaPrivate
        QtMultimediaMockBackend
)
