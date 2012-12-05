TEMPLATE=app
TARGET=declarative-camera

QT += quick qml multimedia

SOURCES += qmlcamera.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/multimediawidgets/declarative-camera
INSTALLS += target

