# Generated from qmediasource.pro.

#####################################################################
## tst_qmediasource Test:
#####################################################################

qt_add_test(tst_qmediasource
    SOURCES
        tst_qmediasource.cpp
    INCLUDE_DIRECTORIES
        ../../mockbackend
    PUBLIC_LIBRARIES
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}
        Qt::Gui
        Qt::MultimediaPrivate
        QtMultimediaMockBackend
)
