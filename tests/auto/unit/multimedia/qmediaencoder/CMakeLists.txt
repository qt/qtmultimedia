# Generated from QMediaEncoder.pro.

#####################################################################
## tst_QMediaEncoder Test:
#####################################################################

qt_internal_add_test(tst_qmediaencoder
    SOURCES
        tst_qmediaencoder.cpp
    INCLUDE_DIRECTORIES
        ../../mockbackend
    PUBLIC_LIBRARIES
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}
        Qt::Gui
        Qt::MultimediaPrivate
        QtMultimediaMockBackend
)
