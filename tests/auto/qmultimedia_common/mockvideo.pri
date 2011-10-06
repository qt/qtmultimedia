# video related mock backend files
INCLUDEPATH += $$PWD \
    ../../../src/multimedia \
    ../../../src/multimedia/video

contains(QT,multimediawidgets)|contains(QT,multimediawidgets-private) {
    HEADERS *= ../qmultimedia_common/mockvideowindowcontrol.h
    DEFINES *= QT_MULTIMEDIA_MOCK_WIDGETS
}

HEADERS *= \
    ../qmultimedia_common/mockvideosurface.h \
    ../qmultimedia_common/mockvideorenderercontrol.h

