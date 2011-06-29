CONFIG -= qt
TEMPLATE = app

# Input
SOURCES += main.cpp

INCLUDEPATH += $$MW_LAYER_SYSTEMINCLUDE
LIBS += -mediaclientvideo.lib