# video related mock backend files
INCLUDEPATH += $$PWD \
    ../../../src/multimedia \
    ../../../src/multimedia/video

contains(QT,multimediakitwidgets)|contains(QT,multimediakitwidgets-private) {
    HEADERS *= ../qmultimedia_common/mockvideowindowcontrol.h
    DEFINES *= QT_MULTIMEDIAKIT_MOCK_WIDGETS
}

HEADERS *= \
    ../qmultimedia_common/mockvideosurface.h \
    ../qmultimedia_common/mockvideorenderercontrol.h

