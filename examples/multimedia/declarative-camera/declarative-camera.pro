TEMPLATE=app
TARGET=declarative-camera

QT += quick qml multimedia
CONFIG += add_ios_ffmpeg_libraries

SOURCES += qmlcamera.cpp
RESOURCES += declarative-camera.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/multimedia/declarative-camera
INSTALLS += target
include(../shared/shared.pri)

