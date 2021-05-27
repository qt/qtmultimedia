# Generated from qcamera.pro.

#####################################################################
## tst_qcamera Test:
#####################################################################

qt_add_test(tst_qcamera
    SOURCES
        tst_qcamera.cpp
    INCLUDE_DIRECTORIES
        ../../mockbackend
    PUBLIC_LIBRARIES
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}
        Qt::Gui
        Qt::MultimediaPrivate
        QtMultimediaMockBackend
)
