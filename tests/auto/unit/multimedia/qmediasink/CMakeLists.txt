# Generated from qmediasink.pro.

#####################################################################
## tst_qmediasink Test:
#####################################################################

qt_add_test(tst_qmediasink
    SOURCES
        tst_qmediasink.cpp
    INCLUDE_DIRECTORIES
        ../../mockbackend
    PUBLIC_LIBRARIES
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}
        Qt::Gui
        Qt::MultimediaPrivate
        QtMultimediaMockBackend
)
