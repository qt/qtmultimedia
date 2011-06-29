CONFIG -= qt
CONFIG += console
TEMPLATE = app

# Input
SOURCES += main.cpp

LIBS += -lstrmiids -ldmoguids -luuid -lmsdmo -lole32 -loleaut32

