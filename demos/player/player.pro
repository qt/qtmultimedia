TEMPLATE = app
TARGET = player
QT += network \
      xml

INCLUDEPATH += ../../src/multimedia ../../src/multimedia/audio

include(../demos.pri)
CONFIG += mobility
MOBILITY = multimedia

HEADERS = \
    player.h \
    playercontrols.h \
    playlistmodel.h \
    videowidget.h
SOURCES = main.cpp \
    player.cpp \
    playercontrols.cpp \
    playlistmodel.cpp \
    videowidget.cpp

maemo* {
    DEFINES += PLAYER_NO_COLOROPTIONS
}

symbian {
    LIBS += -lavkon -lcone -leikcore
    TARGET.CAPABILITY = ReadUserData
}
