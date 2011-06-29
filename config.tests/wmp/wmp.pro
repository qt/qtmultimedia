CONFIG -= qt
CONFIG += console
TEMPLATE = app

# Input
SOURCES += main.cpp

LIBS += -lstrmiids -lole32 -lOleaut32 -luser32 -lgdi32

