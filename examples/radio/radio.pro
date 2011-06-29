TEMPLATE = app
CONFIG += example

INCLUDEPATH += ../../src/multimedia
include(../mobility_examples.pri)

CONFIG += mobility
MOBILITY = multimedia

QMAKE_RPATHDIR += $$DESTDIR

HEADERS = \
    radio.h
  
SOURCES = \
    main.cpp \
    radio.cpp

symbian: {
    TARGET.CAPABILITY = UserEnvironment WriteDeviceData ReadDeviceData SwEvent
}
