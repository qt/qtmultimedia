CONFIG -= qt
CONFIG += console

SOURCES += main.cpp

!wince: LIBS += -lstrmiids -ldmoguids -luuid -lmsdmo -lole32 -loleaut32
