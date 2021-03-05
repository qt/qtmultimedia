# Generated from qmediarecorder.pro.

#####################################################################
## tst_qmediarecorder Test:
#####################################################################

qt_add_test(tst_qmediarecorder
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
