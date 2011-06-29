TEMPLATE = app
CONFIG += example

INCLUDEPATH += ../../src/multimedia ../../src/multimedia/video
include(../mobility_examples.pri)

CONFIG += mobility
MOBILITY = multimedia

QMAKE_RPATHDIR += $$DESTDIR

HEADERS = \
    videoplayer.h \
    videowidget.h \
    videowidgetsurface.h

SOURCES = \
    main.cpp \
    videoplayer.cpp \
    videowidget.cpp \
    videowidgetsurface.cpp

