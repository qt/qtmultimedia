# distinct from Qt Multimedia
TARGET = QtMultimediaWidgets
QT = core gui multimedia widgets-private
QT_PRIVATE += multimedia-private
qtHaveModule(opengl) {
    QT += openglwidgets
    QT_PRIVATE += opengl
}

HEADERS += \
    qvideowidget_p.h \
    qpaintervideosurface_p.h \
    qtmultimediawidgetdefs.h \
    qvideowidgetcontrol.h \
    qvideowidget.h

SOURCES += \
    qpaintervideosurface.cpp \
    qvideowidgetcontrol.cpp \
    qvideowidget.cpp

qtConfig(graphicsview) {
    SOURCES += qgraphicsvideoitem.cpp
    HEADERS += qgraphicsvideoitem.h
}

include(platform/platform.pri)

load(qt_module)
