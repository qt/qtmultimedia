TEMPLATE = app
TARGET = devices

SOURCES = main.cpp

QT += multimedia
CONFIG += console

target.path = $$[QT_INSTALL_EXAMPLES]/multimedia/devices
INSTALLS += target
