load(qt_module)

TARGET = QtMultimedia
QPRO_PWD = $$PWD
QT = core-private gui

CONFIG += module
MODULE_PRI = ../../modules/qt_multimedia.pri

DEFINES += QT_BUILD_MULTIMEDIA_LIB QT_NO_USING_NAMESPACE

unix|win32-g++*:QMAKE_PKGCONFIG_REQUIRES = QtCore QtGui

load(qt_module_config)

HEADERS += qtmultimediaversion.h

include(audio/audio.pri)
include(video/video.pri)

symbian: {
    TARGET.UID3 = 0x2001E627
}
