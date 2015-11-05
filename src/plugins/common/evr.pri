INCLUDEPATH += $$PWD/evr

qtHaveModule(widgets): QT += widgets

HEADERS += \
    $$PWD/evr/evrvideowindowcontrol.h \
    $$PWD/evr/evrdefs.h

SOURCES += \
    $$PWD/evr/evrvideowindowcontrol.cpp \
    $$PWD/evr/evrdefs.cpp

contains(QT_CONFIG, angle)|contains(QT_CONFIG, dynamicgl) {
    LIBS += -lmf -lmfplat -lmfuuid -ld3d9 -ldxva2 -lwinmm -levr
    QT += gui-private

    DEFINES += CUSTOM_EVR_PRESENTER

    HEADERS += \
        $$PWD/evr/evrcustompresenter.h \
        $$PWD/evr/evrd3dpresentengine.h \
        $$PWD/evr/evrhelpers.h

    SOURCES += \
        $$PWD/evr/evrcustompresenter.cpp \
        $$PWD/evr/evrd3dpresentengine.cpp \
        $$PWD/evr/evrhelpers.cpp
}
