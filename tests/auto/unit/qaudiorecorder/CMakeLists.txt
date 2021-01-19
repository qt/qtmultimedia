# Generated from qaudiorecorder.pro.

#####################################################################
## tst_qaudiorecorder Test:
#####################################################################

qt_add_test(tst_qaudiorecorder
    SOURCES
        ../qmultimedia_common/mockaudioencodercontrol.h
        ../qmultimedia_common/mockaudioprobecontrol.h
        ../qmultimedia_common/mockmediacontainercontrol.h
        ../qmultimedia_common/mockmediarecordercontrol.h
        ../qmultimedia_common/mockmediarecorderservice.h
        ../qmultimedia_common/mockmediaservice.h
        ../qmultimedia_common/mockmediaserviceprovider.h
        ../qmultimedia_common/mockmediasource.h
        ../qmultimedia_common/mockmetadatareadercontrol.h
        ../qmultimedia_common/mockmetadatawritercontrol.h
        ../qmultimedia_common/mockvideoencodercontrol.h
        tst_qaudiorecorder.cpp
    INCLUDE_DIRECTORIES
        ../../../../src/multimedia
        ../../../src/multimedia
        ../../../src/multimedia/audio
        ../../../src/multimedia/video
        ../qmultimedia_common
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
)
