# Generated from qaudiodecoder.pro.

#####################################################################
## tst_qaudiodecoder Test:
#####################################################################

qt_internal_add_test(tst_qaudiodecoder
    SOURCES
        tst_qaudiodecoder.cpp
    INCLUDE_DIRECTORIES
        ../../mockbackend
    PUBLIC_LIBRARIES
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}
        Qt::Gui
        Qt::Multimedia
        Qt::MultimediaPrivate
        QtMultimediaMockBackend
)
