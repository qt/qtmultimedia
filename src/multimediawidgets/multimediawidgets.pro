# distinct from Qt Multimedia
TARGET = QtMultimediaWidgets
QT = core gui multimedia-private widgets-private
qtHaveModule(opengl) {
    QT += openglwidgets
    QT_PRIVATE += opengl
}

HEADERS += \
    qvideowidget_p.h \
    qpaintervideosurface_p.h \
    qtmultimediawidgetdefs.h \
    qvideowidget.h

SOURCES += \
    qpaintervideosurface.cpp \
    qvideowidget.cpp

qtConfig(graphicsview) {
    SOURCES += qgraphicsvideoitem.cpp
    HEADERS += qgraphicsvideoitem.h
}

load(qt_module)
