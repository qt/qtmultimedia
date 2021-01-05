# Generated from qmediasink.pro.

#####################################################################
## tst_qmediasink Test:
#####################################################################

qt_add_test(tst_qmediasink
    SOURCES
        ../qmultimedia_common/mockaudioencodercontrol.h
        ../qmultimedia_common/mockaudioinputselector.h
        ../qmultimedia_common/mockaudioprobecontrol.h
        ../qmultimedia_common/mockmediacontainercontrol.h
        ../qmultimedia_common/mockmediasource.h
        ../qmultimedia_common/mockmediarecordercontrol.h
        ../qmultimedia_common/mockmediarecorderservice.h
        ../qmultimedia_common/mockmediaservice.h
        ../qmultimedia_common/mockmediaserviceprovider.h
        ../qmultimedia_common/mockmetadatareadercontrol.h
        ../qmultimedia_common/mockmetadatawritercontrol.h
        ../qmultimedia_common/mockvideoencodercontrol.h
        tst_qmediasink.cpp
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
