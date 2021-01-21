# Generated from qaudioprobe.pro.

#####################################################################
## tst_qaudioprobe Test:
#####################################################################

qt_add_test(tst_qaudioprobe
    SOURCES
        tst_qaudioprobe.cpp
    INCLUDE_DIRECTORIES
        ../../mockbackend
    PUBLIC_LIBRARIES
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}
        Qt::Gui
        Qt::MultimediaPrivate
        QtMultimediaMockBackend
)
