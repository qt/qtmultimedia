load(qt_module)

TARGET = QtMultimedia
QPRO_PWD = $$PWD
QT = core gui

DEFINES += QT_BUILD_MULTIMEDIA_LIB QT_NO_USING_NAMESPACE

unix|win32-g++*:QMAKE_PKGCONFIG_REQUIRES = QtCore QtGui

include($$QT_SOURCE_TREE/src/qbase.pri)

include(audio/audio.pri)
include(video/video.pri)

symbian: {
    TARGET.UID3 = 0x2001E627
}
