TEMPLATE = app
CONFIG += example

INCLUDEPATH += ../../src/multimedia ../../src/multimedia/audio
include(../mobility_examples.pri)

CONFIG += mobility
MOBILITY = multimedia

QMAKE_RPATHDIR += $$DESTDIR

HEADERS       = audiodevices.h

SOURCES       = audiodevices.cpp \
                main.cpp

FORMS        += audiodevicesbase.ui

symbian {
    TARGET.CAPABILITY = UserEnvironment WriteDeviceData ReadDeviceData
}
