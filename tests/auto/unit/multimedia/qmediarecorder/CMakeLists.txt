# Generated from QMediaRecorder.pro.

#####################################################################
## tst_QMediaRecorder Test:
#####################################################################

qt_internal_add_test(tst_qmediarecorder
    SOURCES
        tst_qmediarecorder.cpp
    INCLUDE_DIRECTORIES
        ../../mockbackend
    PUBLIC_LIBRARIES
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}
        Qt::Gui
        Qt::MultimediaPrivate
        QtMultimediaMockBackend
)
