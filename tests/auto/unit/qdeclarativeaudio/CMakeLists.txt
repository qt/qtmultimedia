# Generated from qdeclarativeaudio.pro.

#####################################################################
## tst_qdeclarativeaudio Test:
#####################################################################

qt_add_test(tst_qdeclarativeaudio
    SOURCES
        ../../../../src/imports/multimedia/qdeclarativeaudio.cpp ../../../../src/imports/multimedia/qdeclarativeaudio_p.h
        ../../../../src/imports/multimedia/qdeclarativemediametadata_p.h
        ../../../../src/imports/multimedia/qdeclarativeplaylist.cpp ../../../../src/imports/multimedia/qdeclarativeplaylist_p.h
        ../qmultimedia_common/mockmediaobject.h
        ../qmultimedia_common/mockmediaplayercontrol.h
        ../qmultimedia_common/mockmediaplayerservice.h
        ../qmultimedia_common/mockmediaservice.h
        ../qmultimedia_common/mockmediaserviceprovider.h
        ../qmultimedia_common/mockmediastreamscontrol.h
        ../qmultimedia_common/mockvideoprobecontrol.h
        ../qmultimedia_common/mockvideorenderercontrol.h
        ../qmultimedia_common/mockvideosurface.h
        ../qmultimedia_common/mockvideowindowcontrol.h
        tst_qdeclarativeaudio.cpp
    INCLUDE_DIRECTORIES
        ../../../../src/imports/multimedia
        ../../../../src/multimedia
        ../../../src/multimedia ../../../src/multimedia
        ../../../src/multimedia/audio
        ../../../src/multimedia/video ../../../src/multimedia/video
        ../qmultimedia_common ../qmultimedia_common
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MultimediaQuickPrivate
        Qt::Qml
)
