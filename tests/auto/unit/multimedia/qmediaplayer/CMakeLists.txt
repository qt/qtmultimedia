# Generated from qmediaplayer.pro.

#####################################################################
## tst_qmediaplayer Test:
#####################################################################

qt_internal_add_test(tst_qmediaplayer
    SOURCES
        tst_qmediaplayer.cpp
    INCLUDE_DIRECTORIES
        ../../mockbackend
    PUBLIC_LIBRARIES
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::Network
        QtMultimediaMockBackend
)

# Resources:
set(testdata_resource_files
    "testdata/nokia-tune.mp3"
)

qt_internal_add_resource(tst_qmediaplayer "testdata"
    PREFIX
        "/"
    FILES
        ${testdata_resource_files}
)

