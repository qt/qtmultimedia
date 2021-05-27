# Generated from qcamerainfo.pro.

#####################################################################
## tst_qcamerainfo Test:
#####################################################################

qt_add_test(tst_qcamerainfo
    SOURCES
        tst_qcamerainfo.cpp
    INCLUDE_DIRECTORIES
        ../../mockbackend
    PUBLIC_LIBRARIES
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}
        Qt::Gui
        Qt::MultimediaPrivate
        QtMultimediaMockBackend
)
