INCLUDEPATH += $$PWD

qtHaveModule(widgets): QT += widgets
QT += gui-private

LIBS += -lmf -lmfplat -lmfuuid -ld3d9 -ldxva2 -lwinmm -levr

HEADERS += \
    $$PWD/evrvideowindowcontrol_p.h \
    $$PWD/evrcustompresenter_p.h \
    $$PWD/evrd3dpresentengine_p.h \
    $$PWD/evrhelpers_p.h \
    $$PWD/evrdefs_p.h

SOURCES += \
    $$PWD/evrvideowindowcontrol.cpp \
    $$PWD/evrcustompresenter.cpp \
    $$PWD/evrd3dpresentengine.cpp \
    $$PWD/evrhelpers.cpp \
    $$PWD/evrdefs.cpp
