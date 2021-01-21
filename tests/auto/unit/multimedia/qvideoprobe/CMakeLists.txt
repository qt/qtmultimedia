# Generated from qvideoprobe.pro.

#####################################################################
## tst_qvideoprobe Test:
#####################################################################

qt_add_test(tst_qvideoprobe
    SOURCES
        tst_qvideoprobe.cpp
    INCLUDE_DIRECTORIES
        ../../mockbackend
    PUBLIC_LIBRARIES
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}
        Qt::Gui
        Qt::MultimediaPrivate
        QtMultimediaMockBackend
)
