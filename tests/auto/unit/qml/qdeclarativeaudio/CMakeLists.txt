# Generated from qdeclarativeaudio.pro.

#####################################################################
## tst_qdeclarativeaudio Test:
#####################################################################

qt_add_test(tst_qdeclarativeaudio
    SOURCES
        ../../../../../src/imports/multimedia/qdeclarativeaudio.cpp ../../../../../src/imports/multimedia/qdeclarativeaudio_p.h
        ../../../../../src/imports/multimedia/qdeclarativemediametadata_p.h
        ../../../../../src/imports/multimedia/qdeclarativeplaylist.cpp ../../../../../src/imports/multimedia/qdeclarativeplaylist_p.h
        tst_qdeclarativeaudio.cpp
    INCLUDE_DIRECTORIES
        ../../../../../src/imports/multimedia
        ../../mockbackend
    PUBLIC_LIBRARIES
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MultimediaQuickPrivate
        Qt::Qml
        QtMultimediaMockBackend
)
