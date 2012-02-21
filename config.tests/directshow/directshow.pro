CONFIG -= qt
CONFIG += console

requires(win32*)

SOURCES += main.cpp

LIBS += -lstrmiids -ldmoguids -luuid -lmsdmo -lole32 -loleaut32
