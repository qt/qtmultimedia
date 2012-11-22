TEMPLATE=app
TARGET=declarative-camera

QT += quick qml multimedia

SOURCES += qmlcamera.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/declarative-camera
sources.files = $$SOURCES *.pro images *.qml
sources.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/declarative-camera
INSTALLS += target sources

