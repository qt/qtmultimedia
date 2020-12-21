# Generated from qdeclarativevideo.pro.

#####################################################################
## tst_qdeclarativevideo Test:
#####################################################################

qt_add_test(tst_qdeclarativevideo
    SOURCES
        ../../../../src/imports/multimedia/qdeclarativemediabase.cpp ../../../../src/imports/multimedia/qdeclarativemediabase_p.h
        ../../../../src/imports/multimedia/qdeclarativemediametadata_p.h
        ../../../../src/imports/multimedia/qdeclarativevideo.cpp ../../../../src/imports/multimedia/qdeclarativevideo_p.h
        tst_qdeclarativevideo.cpp
    DEFINES
        QT_DISABLE_DEPRECATED_BEFORE=0
    INCLUDE_DIRECTORIES
        ../../../../src/imports/multimedia
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::Qml
        Qt::Widgets
)
