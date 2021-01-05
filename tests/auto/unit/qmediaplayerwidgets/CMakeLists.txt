# Generated from qmediaplayerwidgets.pro.

#####################################################################
## tst_qmediaplayerwidgets Test:
#####################################################################

qt_add_test(tst_qmediaplayerwidgets
    SOURCES
        ../qmultimedia_common/mockmediasource.h
        ../qmultimedia_common/mockmediaplayercontrol.h
        ../qmultimedia_common/mockmediaplayerservice.h
        ../qmultimedia_common/mockmediaservice.h
        ../qmultimedia_common/mockmediaserviceprovider.h
        ../qmultimedia_common/mockmediastreamscontrol.h
        ../qmultimedia_common/mockvideoprobecontrol.h
        ../qmultimedia_common/mockvideorenderercontrol.h
        ../qmultimedia_common/mockvideosurface.h
        ../qmultimedia_common/mockvideowindowcontrol.h
        tst_qmediaplayerwidgets.cpp
    INCLUDE_DIRECTORIES
        ../../../../src/multimedia
        ../../../src/multimedia ../../../src/multimedia
        ../../../src/multimedia/audio
        ../../../src/multimedia/video ../../../src/multimedia/video
        ../qmultimedia_common ../qmultimedia_common
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MultimediaWidgetsPrivate
        Qt::Network
        Qt::Widgets
)
