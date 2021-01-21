# Generated from qmetadatareadercontrol.pro.

#####################################################################
## tst_qmetadatareadercontrol Test:
#####################################################################

qt_add_test(tst_qmetadatareadercontrol
    SOURCES
        tst_qmetadatareadercontrol.cpp
    INCLUDE_DIRECTORIES
        ../../mockbackend
    PUBLIC_LIBRARIES
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}
        Qt::Gui
        Qt::MultimediaPrivate
        QtMultimediaMockBackend
)
