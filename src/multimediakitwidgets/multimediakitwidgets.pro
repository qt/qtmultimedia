load(qt_module)

# distinct from QtMultimediaKit
TARGET = QtMultimediaKitWidgets
QPRO_PWD = $$PWD
QT = core gui multimediakit-private

CONFIG += module no_private_qt_headers_warning
MODULE_PRI += ../../modules/qt_multimediakitwidgets.pri

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
    qtmultimediakitwidgetdefs.h \
    qtmultimediakitwidgetsversion.h \
    qcameraviewfinder.h \
    qgraphicsvideoitem.h \
    qvideowidgetcontrol.h \
    qvideowidget.h \
    qvideowindowcontrol.h

SOURCES += \
    qcameraviewfinder.cpp \
    qpaintervideosurface.cpp \
    qvideowidgetcontrol.cpp \
    qvideowidget.cpp \
    qvideowindowcontrol.cpp \

mac:!qpa {
!simulator {
   PRIVATE_HEADERS += qpaintervideosurface_mac_p.h
   OBJECTIVE_SOURCES += qpaintervideosurface_mac.mm
}
   LIBS += -framework AppKit -framework QuartzCore -framework QTKit
}

maemo6 {
    isEqual(QT_ARCH,armv6) {
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

