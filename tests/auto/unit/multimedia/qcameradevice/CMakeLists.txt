# Generated from qcameradevice.pro.

#####################################################################
## tst_qcameradevice Test:
#####################################################################

qt_internal_add_test(tst_qcameradevice
    SOURCES
        tst_qcameradevice.cpp
    INCLUDE_DIRECTORIES
        ../../mockbackend
    PUBLIC_LIBRARIES
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}
        Qt::Gui
        Qt::MultimediaPrivate
        QtMultimediaMockBackend
)
