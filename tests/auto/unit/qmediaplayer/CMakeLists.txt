# Generated from qmediaplayer.pro.

#####################################################################
## tst_qmediaplayer Test:
#####################################################################

qt_add_test(tst_qmediaplayer
    SOURCES
        ../qmultimedia_common/mockmediaplayercontrol.h
        ../qmultimedia_common/mockmediaplayerservice.h
        ../qmultimedia_common/mockmediaservice.h
        ../qmultimedia_common/mockmediaserviceprovider.h
        ../qmultimedia_common/mockmediasource.h
        ../qmultimedia_common/mockmediastreamscontrol.h
        ../qmultimedia_common/mockvideoprobecontrol.h
        ../qmultimedia_common/mockvideorenderercontrol.h
        ../qmultimedia_common/mockvideosurface.h
        ../qmultimedia_common/mockvideowindowcontrol.h
        tst_qmediaplayer.cpp
    INCLUDE_DIRECTORIES
        ../../../../src/multimedia
        ../../../src/multimedia ../../../src/multimedia
        ../../../src/multimedia/audio
        ../../../src/multimedia/video ../../../src/multimedia/video
        ../qmultimedia_common ../qmultimedia_common
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::Network
)

# Resources:
set(testdata_resource_files
    "testdata/nokia-tune.mp3"
)

qt_add_resource(tst_qmediaplayer "testdata"
    PREFIX
        "/"
    FILES
        ${testdata_resource_files}
)

