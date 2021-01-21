# Generated from qmetadatawritercontrol.pro.

#####################################################################
## tst_qmetadatawritercontrol Test:
#####################################################################

qt_add_test(tst_qmetadatawritercontrol
    SOURCES
        tst_qmetadatawritercontrol.cpp
    INCLUDE_DIRECTORIES
        ../../mockbackend
    PUBLIC_LIBRARIES
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}
        Qt::Gui
        Qt::MultimediaPrivate
        QtMultimediaMockBackend
)
