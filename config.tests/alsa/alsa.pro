TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += .

SOURCES = alsatest.cpp

CONFIG -= qt dylib

LIBS+=-lasound

