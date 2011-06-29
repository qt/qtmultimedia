TEMPLATE = app
CONFIG += example

INCLUDEPATH += ../../src/multimedia ../../src/multimedia/audio
include(../mobility_examples.pri)

CONFIG += mobility
MOBILITY = multimedia

QMAKE_RPATHDIR += $$DESTDIR

HEADERS       = audiooutput.h

SOURCES       = audiooutput.cpp \
                main.cpp
