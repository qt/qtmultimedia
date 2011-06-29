TEMPLATE = app
CONFIG += example

INCLUDEPATH += ../../src/multimedia ../../src/multimedia/audio
include(../mobility_examples.pri)

CONFIG += mobility
MOBILITY = multimedia

QMAKE_RPATHDIR += $$DESTDIR

HEADERS = \
    audiorecorder.h
  
SOURCES = \
    main.cpp \
    audiorecorder.cpp

maemo*: {
    FORMS += audiorecorder_small.ui
}else:symbian:contains(S60_VERSION, 3.2)|contains(S60_VERSION, 3.1){
    DEFINES += SYMBIAN_S60_3X
    FORMS += audiorecorder_small.ui
}else {
    FORMS += audiorecorder.ui
}
symbian: {
    TARGET.CAPABILITY = UserEnvironment ReadDeviceData WriteDeviceData
}
