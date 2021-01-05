# Generated from qvideoprobe.pro.

#####################################################################
## tst_qvideoprobe Test:
#####################################################################

qt_add_test(tst_qvideoprobe
    SOURCES
        ../qmultimedia_common/mockaudioencodercontrol.h
        ../qmultimedia_common/mockaudioinputselector.h
        ../qmultimedia_common/mockaudioprobecontrol.h
        ../qmultimedia_common/mockmediacontainercontrol.h
        ../qmultimedia_common/mockmediasource.h
        ../qmultimedia_common/mockmediaplayercontrol.h
        ../qmultimedia_common/mockmediaplayerservice.h
        ../qmultimedia_common/mockmediarecordercontrol.h
        ../qmultimedia_common/mockmediarecorderservice.h
        ../qmultimedia_common/mockmediaservice.h
        ../qmultimedia_common/mockmediaserviceprovider.h
        ../qmultimedia_common/mockmediastreamscontrol.h
        ../qmultimedia_common/mockmetadatareadercontrol.h
        ../qmultimedia_common/mockmetadatawritercontrol.h
        ../qmultimedia_common/mockvideoencodercontrol.h
        ../qmultimedia_common/mockvideoprobecontrol.h
        ../qmultimedia_common/mockvideorenderercontrol.h
        ../qmultimedia_common/mockvideosurface.h
        ../qmultimedia_common/mockvideowindowcontrol.h
        tst_qvideoprobe.cpp
    INCLUDE_DIRECTORIES
        ../../../../src/multimedia
        ../../../src/multimedia ../../../src/multimedia
        ../../../src/multimedia/audio
        ../../../src/multimedia/video ../../../src/multimedia/video
        ../qmultimedia_common ../qmultimedia_common
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
)
