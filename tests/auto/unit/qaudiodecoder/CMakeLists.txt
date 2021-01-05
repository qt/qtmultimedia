# Generated from qaudiodecoder.pro.

#####################################################################
## tst_qaudiodecoder Test:
#####################################################################

qt_add_test(tst_qaudiodecoder
    SOURCES
        ../qmultimedia_common/mockaudiodecodercontrol.h
        ../qmultimedia_common/mockaudiodecoderservice.h
        ../qmultimedia_common/mockmediasource.h
        ../qmultimedia_common/mockmediaservice.h
        ../qmultimedia_common/mockmediaserviceprovider.h
        tst_qaudiodecoder.cpp
    INCLUDE_DIRECTORIES
        ../../../../src/multimedia
        ../../../../src/multimedia/audio
        ../../../src/multimedia
        ../../../src/multimedia/audio
        ../../../src/multimedia/controls
        ../qmultimedia_common ../qmultimedia_common
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Multimedia
        Qt::MultimediaPrivate
)

#### Keys ignored in scope 1:.:.:qaudiodecoder.pro:<TRUE>:
# TEMPLATE = "app"
