# Generated from qcameraimagecapture.pro.

#####################################################################
## tst_qcameraimagecapture Test:
#####################################################################

qt_internal_add_test(tst_qcameraimagecapture
    SOURCES
        tst_qcameraimagecapture.cpp
    INCLUDE_DIRECTORIES
        ../../mockbackend
    PUBLIC_LIBRARIES
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}
        Qt::Gui
        Qt::MultimediaPrivate
        QtMultimediaMockBackend
)
