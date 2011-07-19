CONFIG -= qt
CONFIG += console
TEMPLATE = app

requires(win32*)

# Input
SOURCES += main.cpp

LIBS += -lstrmiids -ldmoguids -luuid -lmsdmo -lole32 -loleaut32

