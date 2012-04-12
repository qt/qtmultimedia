load(qt_module)

# distinct from QtMultimedia
TARGET = QtMultimediaWidgets
QPRO_PWD = $$PWD
QT = core gui multimedia-private widgets-private

CONFIG += module no_private_qt_headers_warning
MODULE_PRI += ../../modules/qt_multimediawidgets.pri

contains(QT_CONFIG, opengl) | contains(QT_CONFIG, opengles2) {
   QT += opengl
} else {
   DEFINES += QT_NO_OPENGL
}

!static:DEFINES += QT_MAKEDLL
DEFINES += QT_BUILD_MULTIMEDIAWIDGETS_LIB

load(qt_module_config)

PRIVATE_HEADERS += \
    qvideowidget_p.h \
    qpaintervideosurface_p.h \

PUBLIC_HEADERS += \
    qtmultimediawidgetdefs.h \
    qtmultimediawidgetsversion.h \
    qcameraviewfinder.h \
    qgraphicsvideoitem.h \
    qvideowidgetcontrol.h \
    qvideowidget.h

SOURCES += \
    qcameraviewfinder.cpp \
    qpaintervideosurface.cpp \
    qvideowidgetcontrol.cpp \
    qvideowidget.cpp

mac {
    # QtWidgets is not yet supported on Mac (!).
    false:!simulator {
        PRIVATE_HEADERS += qpaintervideosurface_mac_p.h
        OBJECTIVE_SOURCES += qpaintervideosurface_mac.mm
    }
    LIBS += -framework AppKit -framework QuartzCore -framework QTKit
}

maemo6 {
    contains(QT_CONFIG, opengles2) {
        PRIVATE_HEADERS += qeglimagetexturesurface_p.h
        SOURCES += qeglimagetexturesurface.cpp

        SOURCES += qgraphicsvideoitem_maemo6.cpp

        LIBS += -lX11
    } else {
        SOURCES += qgraphicsvideoitem.cpp
    }
}

!maemo* {
    SOURCES += qgraphicsvideoitem.cpp
}

HEADERS += $$PUBLIC_HEADERS $$PRIVATE_HEADERS

