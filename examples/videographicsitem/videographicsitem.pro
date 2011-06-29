TEMPLATE = app
CONFIG += example

INCLUDEPATH += ../../src/multimedia ../../src/multimedia/video
include(../mobility_examples.pri)

CONFIG += mobility
MOBILITY = multimedia

QMAKE_RPATHDIR += $$DESTDIR

!symbian:contains(QT_CONFIG, opengl): QT += opengl

HEADERS   += videoplayer.h \
             videoitem.h

SOURCES   += main.cpp \
             videoplayer.cpp \
             videoitem.cpp
