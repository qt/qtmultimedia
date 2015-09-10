INCLUDEPATH += $$PWD/evr

qtHaveModule(widgets): QT += widgets

HEADERS += $$PWD/evr/evrvideowindowcontrol.h \
           $$PWD/evr/evrdefs.h

SOURCES += $$PWD/evr/evrvideowindowcontrol.cpp
